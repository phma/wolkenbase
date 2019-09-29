/******************************************************/
/*                                                    */
/* flowsnake.h - flowsnake curve                      */
/*                                                    */
/******************************************************/
/* The flowsnake is a sequence of all Eisenstein integers, where each is
 * followed by an adjacent number. This program proceeds through the
 * point cloud in flowsnake order to minimize thrashing of the octree.
 *
 *      5 6
 *     2 3 4 5 6
 *  5 6.0 1.2 3 4
 * 2 3 4.5 6.0 1
 *  0 1.2 3 4.5 6
 *   5 6.0 1.2 3 4
 *  2 3 4 5 6 0 1
 *   0 1 2 3 4
 *        5 6
 *
 *      5 5
 *     5 5 5 6 6
 *  2 2 5 5 6 6 6
 * 2 2 2 3 3 6 6
 *  2 2 3 3 3 4 4
 *   0 0 3 3 4 4 4
 *  0 0 0 1 1 4 4
 *   0 0 1 1 1
 *        1 1
 *
 *      o o
 *     o o o o o
 *  o o o o o o o
 * o o o 1 2 o o
 *  o o 0 4 3 6 5
 *   3 6 5 6 0 2 4
 *  2 4 5 5 4 1 3
 *   1 0 6 2 3
 *        1 0
 *
 *      5 5
 *     5 5 5 4 4
 *  6 6 5 5 4 4 4
 * 6 6 6 2 2 4 4
 *  6 6 2 2 2 3 3
 *   1 1 2 2 3 3 3
 *  1 1 1 0 0 3 3
 *   1 1 0 0 0
 *        0 0
 *
 */
