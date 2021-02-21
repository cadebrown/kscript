/* vecadd.c - Vector addition kernel
 *
 * Used to show structures and dynamic loading in the `ffi` module
 *   of kscript
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

/* vec3f - Vector containing 3 'float' components
 */
typedef struct {
    float x, y, z;
} vec3f;

/* vecadd - Adds vectors, computes: A = B + C
 */
void vecadd(int N, vec3f* A, vec3f* B, vec3f* C) {
    int i;
    for (i = 0; i < N; ++i) {
        A[i].x = B[i].x + C[i].x;
        A[i].y = B[i].y + C[i].y;
        A[i].z = B[i].z + C[i].z;
    }
}


