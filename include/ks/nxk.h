/* ks/nxk.h - NumeriX kernel generator
 * 
 * This is a special header that includes other files dynamically, based on the macro
 *   'NXK_FILE'
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */


/** Check requested configuration **/

#ifndef NXK_FILE
  #error Must define NXK_FILE
#endif

/* Clear any existing state */
#undef NXK_I
#undef NXK_F
#undef NXK_C
#undef NXK_NAME
#undef NXK_TYPE
#undef NXK_DTYPE


/** Generic macros **/

/* Paste 'X' and 'Y' together
 */
#define _NXK_PASTE(X, Y) X##Y
#define NXK_PASTE(X, Y) _NXK_PASTE(X, Y)


/** Variable Macros **/

/* Declares a 1D argument, with a given name
 *
 * After this, you can use 'NXK_GET_1D(_name, _i)' to get an element
 * 
 * Example:
 * NXK_ARG_1D(0, X) declares:
 *   * A 'nx_t' variable called 'X'
 *   * A 'ks_uint' variable called 'pX' (pointer-of-X), which has the 'data' address as an unsigned integer
 *   * A 'ks_cint' variable called 'sX' (stride-of-X), which is the distance, in bytes, between each element
 */
#define NXK_ARG_1D(_idx, _name) \
    nx_t _name = args[_idx]; \
    assert(_name.rank == 1); \
    ks_uint p##_name = (ks_uint)_name.data; \
    ks_cint s##_name = (ks_cint)_name.strides[0]; \

/* Declares a 2D argument, with a given name
 *
 * After this, you can use 'NXK_GET_2D(_name, _i, _j)' to get an element
 * 
 * Example:
 * NXK_ARG_2D(0, X) declares:
 *   * A 'nx_t' variable called 'X'
 *   * A 'ks_uint' variable called 'pX' (pointer-of-X), which has the 'data' address as an unsigned integer
 *   * A 'ks_cint' variable called 'srX' (stride(row)-of-X), which is the distance, in bytes, between each row
 *   * A 'ks_cint' variable called 'scX' (stride(col)-of-X), which is the distance, in bytes, between each column
 */
#define NXK_ARG_2D(_idx, _name) \
    nx_t _name = args[_idx]; \
    assert(_name.rank == 2); \
    ks_uint p##_name = (ks_uint)_name.data; \
    ks_cint sr##_name = (ks_cint)_name.strides[0]; \
    ks_cint sc##_name = (ks_cint)_name.strides[1]; \

/* Declares a 3D argument, with a given name
 *
 * After this, you can use 'NXK_GET_3D(_name, _i, _j, _k)' to get an element
 * 
 * Example:
 * NXK_ARG_3D(0, X) declares:
 *   * A 'nx_t' variable called 'X'
 *   * A 'ks_uint' variable called 'pX' (pointer-of-X), which has the 'data' address as an unsigned integer
 *   * A 'ks_cint' variable called 'srX' (stride(row)-of-X), which is the distance, in bytes, between each row
 *   * A 'ks_cint' variable called 'scX' (stride(col)-of-X), which is the distance, in bytes, between each column
 *   * A 'ks_cint' variable called 'szX' (stride(Z)-of-X), which is the distance, in bytes, between each slice in the third dimension of 'X'
 */
#define NXK_ARG_3D(_idx, _name) \
    nx_t _name = args[_idx]; \
    assert(_name.rank == 3); \
    ks_uint p##_name = (ks_uint)_name.data; \
    ks_cint sr##_name = (ks_cint)_name.strides[0]; \
    ks_cint sc##_name = (ks_cint)_name.strides[1]; \
    ks_cint sz##_name = (ks_cint)_name.strides[2]; \


/* Gets an element of a variable declared via 'NXK_ARG_1D'
 */
#define NXK_GET_1D(_X, _i) (*(NXK_TYPE*)(p##_X + s##_X * (_i)))

/* Gets an element of a variable declared via 'NXK_ARG_2D', which is the '_i'th row and '_j'th col
 */
#define NXK_GET_2D(_X, _i, _j) (*(NXK_TYPE*)(p##_X + sr##_X * (_i) + sc##_X * (_j)))

/* Gets an element of a variable declared via 'NXK_ARG_2D', which is the '_i'th row and '_j'th col
 */
#define NXK_GET_3D(_X, _i, _j, _k) (*(NXK_TYPE*)(p##_X + sr##_X * (_i) + sc##_X * (_j) + sz##_X * (_k)))



/** Code Generation **/


/* The C type of the current datatype */
#define NXK_TYPE NXK_PASTE(nx_, NXK_NAME)

/* The 'nx_dtype' object representing the current datatype */
#define NXK_DTYPE NXK_PASTE(nxd_, NXK_NAME)

/* Type-specific function */
#define NXK_FUNC(_func) NXK_PASTE(NXK_TYPE, _func)
#define NXK_ATTR(_attr) NXK_PASTE(NXK_TYPE, _attr)

/** Integers **/

#ifdef NXK_DO_I
  #define NXK_DO_B
#endif

#ifdef NXK_DO_B
  #define NXK_B 1

  #define NXK_NAME bl
  #include NXK_FILE
  #undef NXK_NAME
  
  #undef NXK_B
#endif

#ifdef NXK_DO_I
  #define NXK_I 1

  #define NXK_NAME u8
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME s8
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME u16
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME s16
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME u32
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME s32
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME u64
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME s64
  #include NXK_FILE
  #undef NXK_NAME

  #undef NXK_I
#endif /* NXK_DO_I */

/** Floats **/

#ifdef NXK_DO_F
  #define NXK_F 1

  #define NXK_NAME S
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME D
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME E
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME Q
  #include NXK_FILE
  #undef NXK_NAME

  #undef NXK_F
#endif /* NXK_DO_F */

/** Complex-floats **/

#ifdef NXK_DO_C
  #define NXK_C 1

  #define NXK_NAME cS
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME cD
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME cE
  #include NXK_FILE
  #undef NXK_NAME
  #define NXK_NAME cQ
  #include NXK_FILE
  #undef NXK_NAME

  #undef NXK_C
#endif /* NXK_DO_C */


#undef NXK_DO_I
#undef NXK_DO_F
#undef NXK_DO_C

#undef NXK_FILE
