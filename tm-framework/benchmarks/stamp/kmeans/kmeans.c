/* =============================================================================
 *
 * kmeans.c
 *
 * =============================================================================
 *
 * Description:
 *
 * Takes as input a file:
 *   ascii  file: containing 1 data point per line
 *   binary file: first int is the number of objects
 *                2nd int is the no. of features of each object
 *
 * This example performs a fuzzy c-means clustering on the data. Fuzzy clustering
 * is performed using min to max clusters and the clustering that gets the best
 * score according to a compactness and separation criterion are returned.
 *
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department Northwestern University
 * email: wkliao@ece.northwestern.edu
 *
 *
 * Edited by:
 *
 * Jay Pisharath
 * Northwestern University
 *
 * Chi Cao Minh
 * Stanford University
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cluster.h"
#include "common.h"
#include "thread.h"
#include "tm.h"
#include "util.h"

#define MAX_LINE_LENGTH 1000000	/* max input is 400000 one digit input + spaces */

extern double global_time;

unsigned int htm_rot_enabled = 1;
unsigned int allow_rots_ros = 1;
unsigned int allow_htms = 1;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
padded_scalar_t fallback_in_use;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
padded_scalar_t exists_sw;

__thread unsigned long backoff = MIN_BACKOFF;
__thread unsigned long cm_seed = 123456789UL;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
padded_statistics_t stats_array[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
pthread_spinlock_t single_global_lock = 0;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
padded_scalar_t counters[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
pthread_spinlock_t writers_lock = 0;

__attribute__ ((aligned (CACHE_LINE_SIZE)))
pthread_spinlock_t reader_locks[80];

__attribute__ ((aligned (CACHE_LINE_SIZE)))
padded_statistics_t statistics_array[80];

__thread unsigned int local_exec_mode = 0;

__thread unsigned int local_thread_id;

__thread void *rot_readset[100000];
__thread unsigned long rs_counter = 0;


unsigned int ucb_levers = 3;
unsigned long ucb_trials[3];


/* =============================================================================
 * usage
 * =============================================================================
 */
void usage (char *argv0)
{
    char *help =
        "Usage: %s [switches] -i filename\n"
        "       -i filename:     file containing data to be clustered\n"
        "       -b               input file is in binary format\n"
        "       -m max_clusters: maximum number of clusters allowed\n"
        "       -n min_clusters: minimum number of clusters allowed\n"
        "       -z             : don't zscore transform data\n"
        "       -t threshold   : threshold value\n"
        "       -r repeat      : number of repetitions\n"
        "       -p nproc       : number of threads\n";
    fprintf (stderr, help, argv0);
    exit (-1);
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN (argc, argv)
{
    int max_nclusters = 13;
    int min_nclusters = 4;
    char *filename = 0;
    REAL *buf;
    REAL **attributes;
    REAL **cluster_centres = NULL;
    int i;
    int j;
    int best_nclusters;
    int *cluster_assign;
    int numAttributes;
    int numObjects;
    int use_zscore_transform = 1;
    char *line;
    int isBinaryFile = 0;
    int nloops;
    int repeats = 1;
    int len;
    int nthreads;
    REAL threshold = 0.001;
    int opt;

    SETUP_NUMBER_TASKS (3);

    GOTO_REAL  ();

    line = (char *) malloc (MAX_LINE_LENGTH);	/* reserve memory line */

    nthreads = 1;
    while ((opt = getopt (argc, (char **) argv, "p:i:m:n:r:t:bz")) != EOF)
    {
        switch (opt)
        {
        case 'r':
            repeats = atoi (optarg);
            break;
        case 'i':
            filename = optarg;
            break;
        case 'b':
            isBinaryFile = 1;
            break;
        case 't':
            threshold = atof (optarg);
            break;
        case 'm':
            max_nclusters = atoi (optarg);
            break;
        case 'n':
            min_nclusters = atoi (optarg);
            break;
        case 'z':
            use_zscore_transform = 0;
            break;
        case 'p':
            nthreads = atoi (optarg);
            break;
        case '?':
            usage ((char *) argv[0]);
            break;
        default:
            usage ((char *) argv[0]);
            break;
        }
    }

    if (filename == 0)
    {
        usage ((char *) argv[0]);
    }

    if (max_nclusters < min_nclusters)
    {
        fprintf (stderr, "Error: max_clusters must be >= min_clusters\n");
        usage ((char *) argv[0]);
    }

	SETUP_NUMBER_THREADS (nthreads);

	TM_STARTUP (nthreads);
	thread_startup (nthreads);

	for (; repeats > 0; --repeats)
	{
		numAttributes = 0;
		numObjects = 0;

		/*
		 * From the input file, get the numAttributes and numObjects
		 */
		if (isBinaryFile)
		{
			int infile;

			if ((infile = open (filename, O_RDONLY, "0600")) == -1)
			{
				fprintf (stderr, "Error: no such file (%s)\n", filename);
				exit (1);
			}
			read (infile, &numObjects, sizeof (int));
			read (infile, &numAttributes, sizeof (int));

			/* Allocate space for attributes[] and read attributes of all objects */
			buf =
				(REAL *) malloc (numObjects * numAttributes * sizeof (float));
			assert (buf);
			attributes = (REAL **) malloc (numObjects * sizeof (REAL *));
			assert (attributes);
			attributes[0] =
				(REAL *) malloc (numObjects * numAttributes * sizeof (float));
			assert (attributes[0]);
			for (i = 1; i < numObjects; i++)
			{
				attributes[i] = attributes[i - 1] + numAttributes;
			}
			read (infile, buf, (numObjects * numAttributes * sizeof (float)));
			close (infile);
		}
		else
		{
			FILE *infile;

			if ((infile = fopen (filename, "r")) == NULL)
			{
				fprintf (stderr, "Error: no such file (%s)\n", filename);
				exit (1);
			}
			while (fgets (line, MAX_LINE_LENGTH, infile) != NULL)
			{
				if (strtok (line, " \t\n") != 0)
				{
					numObjects++;
				}
			}
			rewind (infile);
			while (fgets (line, MAX_LINE_LENGTH, infile) != NULL)
			{
				if (strtok (line, " \t\n") != 0)
				{
					/* Ignore the id (first attribute): numAttributes = 1; */
					while (strtok (NULL, " ,\t\n") != NULL)
					{
						numAttributes++;
					}
					break;
				}
			}

            /* Allocate space for attributes[] and read attributes of all objects */
            buf = (REAL*) malloc (numObjects * numAttributes * sizeof (REAL));
            assert (buf);
            attributes = (REAL**) malloc (numObjects * sizeof (REAL*));
            assert (attributes);
            attributes[0] =
                (REAL *) malloc (numObjects * numAttributes * sizeof (REAL));
            assert (attributes[0]);
            for (i = 1; i < numObjects; i++)
            {
                attributes[i] = attributes[i - 1] + numAttributes;
            }
            rewind (infile);
            i = 0;
            while (fgets (line, MAX_LINE_LENGTH, infile) != NULL)
            {
                if (strtok (line, " \t\n") == NULL)
                {
                    continue;
                }
                for (j = 0; j < numAttributes; j++)
                {
                    buf[i] = atof (strtok (NULL, " ,\t\n"));
                    i++;
                }
            }
			fclose (infile);
        }

        /*
         * The core of the clustering
         */
        cluster_assign = (INT*) malloc (numObjects * sizeof (INT));
        assert (cluster_assign);

        nloops = 1;
        len = max_nclusters - min_nclusters + 1;

        for (i = 0; i < nloops; i++)
        {
            /*
             * Since zscore transform may perform in cluster() which modifies the
             * contents of attributes[][], we need to re-store the originals
             */
            memcpy (attributes[0], buf,
                    (numObjects * numAttributes * sizeof (REAL)));

            cluster_centres = NULL;
            cluster_exec (nthreads, numObjects, numAttributes, attributes,	/* [numObjects][numAttributes] */
                          use_zscore_transform,	/* 0 or 1 */
                          min_nclusters,	/* pre-define range from min to max */
                          max_nclusters, threshold, &best_nclusters,	/* return: number between min and max */
                          &cluster_centres,	/* return: [best_nclusters][numAttributes] */
                          cluster_assign);	/* return: [numObjects] cluster id for each object */

        }
    }

    free (cluster_assign);
    free (attributes);
    if (cluster_centres != NULL) {
        free (cluster_centres[0]);
        free (cluster_centres);
    }
    // free (buf);

    printf ("Time: %lg seconds\n", global_time);

    TM_SHUTDOWN ();

    GOTO_SIM ();

    thread_shutdown ();

    MAIN_RETURN (0);
}


/* =============================================================================
 *
 * End of kmeans.c
 *
 * =============================================================================
 */
