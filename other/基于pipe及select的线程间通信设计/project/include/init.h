#ifndef __INIT_H__
#define __INIT_H__

typedef enum
{
    /* initiaze the resource only depend on OS,
       the other moudule interface can not be called in this phase */
    INIT_PHASE_INSIDE = 0,

    /* restore the data from flash*/
    INIT_PHASE_RESTORE,

    /* the other moudule interface can be called in this phase,
       such as register callback,
       get the configuration parameter from configuration module,
       create timer */
    INIT_PHASE_OUTSIDE,

    INIT_PHASE_COUNT,
} INIT_PHASE_TYPE;

#endif
