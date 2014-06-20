/***********************************************************************
* std_djc.h                                                            *
*                                                                      *
* Standard macros I use                                                *
*                                                                      *
***********************************************************************/


#ifndef STD_DJC_H
#define STD_DJC_H

/* A simple macro that I can give a condition and have it jump to an err: handler if the condition is true */
#define err_chk(cond) if(cond) { goto err; }

#endif
