/****************************************************************************
 *
 * convex-hull.c
 *
 * Compute the convex hull of a set of points in 2D
 *
 * Copyright (C) 2019 Moreno Marzolla <moreno.marzolla(at)unibo.it>
 * Last updated on 2019-11-25
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 *
 * Questo programma calcola l'inviluppo convesso (convex hull) di un
 * insieme di punti 2D letti da standard input usando l'algoritmo
 * "gift wrapping". Le coordinate dei vertici dell'inviluppo sono
 * stampate su standard output.  Per una descrizione completa del
 * problema si veda la specifica del progetto sul sito del corso:
 *
 * http://moreno.marzolla.name/teaching/HPC/
 *
 * Per compilare:
 *
 * gcc -D_XOPEN_SOURCE=600 -std=c99 -Wall -Wpedantic -O2 convex-hull.c -o convex-hull -lm
 *
 * (il flag -D_XOPEN_SOURCE=600 e' superfluo perche' viene settato
 * nell'header "hpc.h", ma definirlo tramite la riga di comando fa si'
 * che il programma compili correttamente anche se non si include
 * "hpc.h", o per errore non lo si include come primo file).
 *
 * Per eseguire il programma si puo' usare la riga di comando:
 *
 * ./convex-hull < ace.in > ace.hull
 * 
 * Per visualizzare graficamente i punti e l'inviluppo calcolato è
 * possibile usare lo script di gnuplot (http://www.gnuplot.info/)
 * incluso nella specifica del progetto:
 *
 * gnuplot -c plot-hull.gp ace.in ace.hull ace.png
 *
 ****************************************************************************/
#include "hpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <mpi.h>
#include <limits.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* A single point */
typedef struct {
    double x, y;
} point_t;

/* An array of n points */
typedef struct {
    int n;      /* number of points     */
    point_t *p; /* array of points      */
} points_t;

enum {
    LEFT = -1,
    COLLINEAR,
    RIGHT
};

/**
 * Read input from file f, and store the set of points into the
 * structure pset.
 */
void read_input( FILE *f, points_t *pset )
{
    char buf[1024];
    int i, dim, npoints;
    point_t *points;

    if ( 1 != fscanf(f, "%d", &dim) ) {
        fprintf(stderr, "FATAL: can not read dimension\n");
        exit(EXIT_FAILURE);
    }
    if (dim != 2) {
        fprintf(stderr, "FATAL: This program supports dimension 2 only (got dimension %d instead)\n", dim);
        exit(EXIT_FAILURE);
    }
    if (NULL == fgets(buf, sizeof(buf), f)) { /* ignore rest of the line */
        fprintf(stderr, "FATAL: failed to read rest of first line\n");
        exit(EXIT_FAILURE);
    }
    if (1 != fscanf(f, "%d", &npoints)) {
        fprintf(stderr, "FATAL: can not read number of points\n");
        exit(EXIT_FAILURE);
    }
    assert(npoints > 2);
    points = (point_t*)malloc( npoints * sizeof(*points) );
    assert(points);
    for (i=0; i<npoints; i++) {
        if (2 != fscanf(f, "%lf %lf", &(points[i].x), &(points[i].y))) {
            fprintf(stderr, "FATAL: failed to get coordinates of point %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    pset->n = npoints;
    pset->p = points;
}

/**
 * Free the memory allocated by structure pset.
 */
void free_pointset( points_t *pset )
{
    pset->n = 0;
    free(pset->p);
    pset->p = NULL;
}

/**
 * Dump the convex hull to file f. The first line is the number of
 * dimensione (always 2); the second line is the number of vertices of
 * the hull PLUS ONE; the next (n+1) lines are the vertices of the
 * hull, in clockwise order. The first point is repeated twice, in
 * order to be able to plot the result using gnuplot as a closed
 * polygon
 */
void write_hull( FILE *f, const points_t *hull )
{
    int i;
    fprintf(f, "%d\n%d\n", 2, hull->n + 1);
    for (i=0; i<hull->n; i++) {
        fprintf(f, "%f %f\n", hull->p[i].x, hull->p[i].y);
    }
    /* write again the coordinates of the first point */
    fprintf(f, "%f %f\n", hull->p[0].x, hull->p[0].y);    
}

/**
 * Return LEFT, RIGHT or COLLINEAR depending on the shape
 * of the vectors p0p1 and p1p2
 *
 * LEFT            RIGHT           COLLINEAR
 * 
 *  p2              p1----p2            p2
 *    \            /                   /
 *     \          /                   /
 *      p1       p0                  p1
 *     /                            /
 *    /                            /
 *  p0                            p0
 *
 * See Cormen, Leiserson, Rivest and Stein, "Introduction to Algorithms",
 * 3rd ed., MIT Press, 2009, Section 33.1 "Line-Segment properties"
 */
int turn(const point_t p0, const point_t p1, const point_t p2)
{
    /*
      This function returns the correct result (COLLINEAR) also in the
      following cases:
      - p0==p1==p2
      - p0==p1
      - p1==p2
    */
    const double cross = (p1.x-p0.x)*(p2.y-p0.y) - (p2.x-p0.x)*(p1.y-p0.y);
    if (cross > 0.0) {
        return LEFT;
    } else {
        if (cross < 0.0) {
            return RIGHT;
        } else {
            return COLLINEAR;
        }
    }
}

/*
    Check if the provided condition is true. If not, gracefully aborts the execution
*/
void myAssert(int const condition) {
    if(!condition) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

/**
 * Compute the convex hull of all points in pset using the "Gift
 * Wrapping" algorithm. The vertices are stored in the hull data
 * structure, that does not need to be initialized by the caller.
 */
void convex_hull(int const rank, int const size, const points_t *pset, points_t *hull)
{
    const int n = pset->n;
    int const start = (rank * n)/size, end = ((rank + 1) * n)/size;
    const point_t *p = pset->p;
    int i, j;
    int cur, next, leftmost;
    int *sendBuf, *recvBuf;
    //hull_tag[i] = 0 if the i-th point is not currently considered part of the hull, 0 otherwise
    int hull_tag[n];
    //buffer in which to hold the results of processes operations
    recvBuf = (int*) malloc(sizeof(int) * size);
    memset(hull_tag, 0, n * sizeof(int));
    if(rank == 0) {
        /* There can be at most n points in the convex hull. At the end of
        this function we trim the excess space. */
        hull->n = 0;
        hull->p = (point_t*)malloc(n * sizeof(*(hull->p))); assert(hull->p);
    }

    /* Identify the leftmost point p[leftmost] */
    leftmost = 0;
    for (i = start; i < end; i++) {
        if (p[i].x < p[leftmost].x) {
            leftmost = i;
        }
    }
    sendBuf = &leftmost;

    MPI_Allgather(sendBuf, 1, MPI_INT, recvBuf, 1, MPI_INT, MPI_COMM_WORLD);
    for (j = 0; j < size; j++) {
        if (p[recvBuf[j]].x < p[leftmost].x) {
            leftmost = recvBuf[j];
        }
    }

    cur = leftmost;

    /* Main loop of the Gift Wrapping algorithm. This is where most of
       the time is spent; therefore, this is the block of code that
       must be parallelized. */

    do {
        /* Add the current vertex to the hull */
        if(rank == 0) {
            myAssert(hull->n < n);
            hull->p[hull->n] = p[cur];
            hull->n++;
        }

        /* find localNext in range [start, end] */
        next = (cur + 1) % n;
        for (j = start; j < end; j++) {
            if (hull_tag[j] == 0)
            {
                if (LEFT == turn(p[cur], p[next], p[j])) {
                    next = j;
                }
            }
        }
        sendBuf = &next;

        /* Process 0 gathers all local next, then finds the global best */
        MPI_Allgather(sendBuf, 1, MPI_INT, recvBuf, 1, MPI_INT, MPI_COMM_WORLD);
        for (j = 0; j < size; j++) {
            if (LEFT == turn(p[cur], p[next], p[recvBuf[j]])) {
                next = recvBuf[j];
            }
        }
        /* Process 0 broadcasts the global next to the other processes */

        myAssert(cur != next);
        cur = next;
        hull_tag[next] = 1;
    } while (cur != leftmost);
    free(recvBuf);
    if(rank == 0) {
        /* Trim the excess space in the convex hull array */
        hull->p = (point_t*)realloc(hull->p, (hull->n) * sizeof(*(hull->p)));
        myAssert(hull->p != NULL);
    }
}

/**
 * Compute the area ("volume", in qconvex terminoloty) of a convex
 * polygon whose vertices are stored in pset using Gauss' area formula
 * (also known as the "shoelace formula"). See:
 *
 * https://en.wikipedia.org/wiki/Shoelace_formula
 *
 * This function does not need to be parallelized.
 */
double hull_volume( const points_t *hull )
{
    const int n = hull->n;
    const point_t *p = hull->p;
    double sum = 0.0;
    int i;
    for (i=0; i<n-1; i++) {
        sum += ( p[i].x * p[i+1].y - p[i+1].x * p[i].y );
    }
    sum += p[n-1].x*p[0].y - p[0].x*p[n-1].y;
    return 0.5*fabs(sum);
}

/**
 * Compute the length of the perimeter ("facet area", in qconvex
 * terminoloty) of a convex polygon whose vertices are stored in pset.
 * This function does not need to be parallelized.
 */
double hull_facet_area( const points_t *hull )
{
    const int n = hull->n;
    const point_t *p = hull->p;
    double length = 0.0;
    int i;
    for (i=0; i<n-1; i++) {
        length += hypot( p[i].x - p[i+1].x, p[i].y - p[i+1].y );
    }
    /* Add the n-th side connecting point n-1 to point 0 */
    length += hypot( p[n-1].x - p[0].x, p[n-1].y - p[0].y );
    return length;
}

int main( int argc, char** argv )
{
    points_t pset, hull;
    int rank, size;
    double tstart, elapsed;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0) {
        read_input(stdin, &pset);
    }

    //Only process 0 has information regarding pset, they have to be broadcasted to the other processes
    //Defining a datatype to map the point_t struct
    MPI_Datatype MPI_point_t;
    MPI_Type_contiguous(2, MPI_DOUBLE, &MPI_point_t);
    MPI_Type_commit(&MPI_point_t);

    MPI_Bcast(&pset.n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        pset.p = (point_t*) malloc(pset.n * sizeof(point_t));
    }
    MPI_Bcast(pset.p, pset.n, MPI_point_t, 0, MPI_COMM_WORLD);

    tstart = hpc_gettime();
    convex_hull(rank, size, &pset, &hull);
    elapsed = hpc_gettime() - tstart;

    if (rank == 0) {
        fprintf(stderr, "\nConvex hull of %d points in 2-d:\n\n", pset.n);
        fprintf(stderr, "  Number of vertices: %d\n", hull.n);
        fprintf(stderr, "  Total facet area: %f\n", hull_facet_area(&hull));
        fprintf(stderr, "  Total volume: %f\n\n", hull_volume(&hull));
        fprintf(stderr, "Elapsed time: %f\n\n", elapsed);
        write_hull(stdout, &hull);
        free_pointset(&pset);
    }
    free_pointset(&hull);
    MPI_Finalize();
    return EXIT_SUCCESS;    
}
