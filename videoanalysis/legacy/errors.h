#ifndef ERRORS_H
#define ERRORS_H

#include <time.h>
#include <glib.h>

enum param_id { BLACK, LUMA, FREEZE, DIFF, BLOCKY, PARAM_NUMBER };

struct param_avg {
        float min;
        float max;
        float avg;
};

struct avg_container {
        struct param_avg avg;
        gboolean empty;
};

struct boundary {
        gboolean cont_en;
        gfloat   cont;
        gboolean peak_en;
        gfloat   peak;
        gfloat   duration;
};

struct container {
        int sz;
        int counter;
        float  denom;
        float* values[PARAM_NUMBER];
        struct avg_container avgs[PARAM_NUMBER];
};

void container_init  (struct container*, int, float);
void container_reset (struct container*);
void container_add   (struct container*, enum param_id, float);


#endif
