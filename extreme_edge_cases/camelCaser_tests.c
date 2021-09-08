/**
 * extreme_edge_cases
 * CS 241 - Fall 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!
    char **rt = 0;


    rt = (*camelCaser)("");
    if (rt[0] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("thisis madddddddddddddddd"); 
    if (rt[0] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);



    rt = (*camelCaser)("/,.>?");
    if (strcmp(rt[0], "") != 0) {
        destroy(rt);
        return 0;
    }
    if (strcmp(rt[1], "") != 0) {
        destroy(rt);
        return 0;
    }
    if (strcmp(rt[2], "") != 0) {
        destroy(rt);
        return 0;
    }

    if (strcmp(rt[3], "") != 0) {
        destroy(rt);
        return 0;
    }

    if (strcmp(rt[4], "") != 0) {
        destroy(rt);
        return 0;
    }


    if (rt[5] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);


    rt = (*camelCaser)("CS241 is Literally mad.");
    if (strcmp(rt[0], "cs241IsLiterallyMad") != 0) {
        destroy(rt);
        return 0;
    }
    if (rt[1] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("WWw. WHWHWH nmmi. TREfefdds.");
    if (strcmp(rt[0], "wwww") != 0) {
        destroy(rt);
        return 0;
    }
    if (strcmp(rt[1], "whwhwhNmmi") != 0) {
        destroy(rt);
        return 0;
    }
    if (strcmp(rt[2], "trefefdds") != 0) {
        destroy(rt);
        return 0;
    }

    if (rt[3] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);


    rt = (*camelCaser)("241Nonsense.");
    if (strcmp(rt[0], "241nonsense") != 0) {
        destroy(rt);
        return 0;
    }
    if (rt[1] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("FFFFF.  WHATAahtata is this  \n for?");
    if (strcmp(rt[0], "fffff") != 0) {
        destroy(rt);
        return 0;
    }

    if (strcmp(rt[1], "whataahtataIsThisFor") != 0) {
        destroy(rt);
        return 0;
    }

    if (rt[2] != NULL) {
        destroy(rt);
        return 0;
    }

    destroy(rt);

    rt = (*camelCaser)("变态中文测试.   Chinese test."); 
    if (strcmp(rt[0], "变态中文测试") != 0) {
        destroy(rt);
        return 0;
    }

    if (strcmp(rt[1], "chineseTest") != 0) {
        destroy(rt);
        return 0;
    }
    
    if (rt[2] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)(".Mad tEst.");
    if (strcmp(rt[0], "") != 0) {
        destroy(rt);
        return 0;
    }

    if (strcmp(rt[1], "madTest") != 0) {
        destroy(rt);
        return 0;
    }

    if (rt[2] != NULL) {
        destroy(rt);
        return 0;
    }

    destroy(rt);

    
    return 1;
}