
#pragma once
#ifndef IR_UNIFORM_H
#define IR_UNIFORM_H


/* stdbool.h is necessary because this file is included in both C and C++ code.
 */
#include "program/prog_parameter.h"  /* For union gl_constant_value. */


enum gl_uniform_driver_format {
   uniform_native = 0,          /**< Store data in the native format. */
   uniform_int_float,           /**< Store integer data as floats. */
   uniform_bool_float,          /**< Store boolean data as floats. */

   /**
    * Store boolean data as integer using 1 for \c true.
    */
   uniform_bool_int_0_1,

   /**
    * Store boolean data as integer using ~0 for \c true.
    */
   uniform_bool_int_0_not0
};

struct gl_uniform_driver_storage {
   /**
    * Number of bytes from one array element to the next.
    */
   uint8_t element_stride;

   /**
    * Number of bytes from one vector in a matrix to the next.
    */
   uint8_t vector_stride;

   /**
    * Base format of the stored data.
    *
    * This field must have a value from \c GLSL_TYPE_UINT through \c
    * GLSL_TYPE_SAMPLER.
    */
   uint8_t format;

   /**
    * Pointer to the base of the data.
    */
   void *data;
};

struct gl_uniform_storage {
   char *name;
   const struct glsl_type *type;

   /**
    * The number of elements in this uniform.
    *
    * For non-arrays, this is always 0.  For arrays, the value is the size of
    * the array.
    */
   unsigned array_elements;

   /**
    * Has this uniform ever been set?
    */
   bool initialized;

   /**
    * Base sampler index
    *
    * If \c ::base_type is \c GLSL_TYPE_SAMPLER, this represents the index of
    * this sampler.  If \c ::array_elements is not zero, the array will use
    * sampler indexes \c ::sampler through \c ::sampler + \c ::array_elements
    * - 1, inclusive.
    */
   uint8_t sampler;

   /**
    * Storage used by the driver for the uniform
    */
   unsigned num_driver_storage;
   struct gl_uniform_driver_storage *driver_storage;

   /**
    * Storage used by Mesa for the uniform
    *
    * This form of the uniform is used by Mesa's implementation of \c
    * glGetUniform.  It can also be used by drivers to obtain the value of the
    * uniform if the \c ::driver_storage interface is not used.
    */
   union gl_constant_value *storage;
};

#endif /* IR_UNIFORM_H */
