#ifndef UTILS_H
#define UTILS_H

#define container_of(PTR, TYPE, FIELD) ({                  \
    __typeof__(((TYPE *)0)->FIELD) *__FIELD_PTR = (PTR);   \
    (TYPE *)((char *)__FIELD_PTR - offsetof(TYPE, FIELD)); \
})

#endif