#ifndef  DEFINES_H_
#define  DEFINES_H_


/*=================================
 *    Debug Condition
 ==================================*/
//#define DEBUG_ZBEESTACK

/*=================================
      Debug Print functions
 ==================================*/
#ifdef  DEBUG_ZBEESTACK
  #define D_ZBEESTACK(...)  printf(__VA_ARGS__)
#else
  #define D_ZBEESTACK(...)
#endif


#endif   /* DEFINES_H_ */
