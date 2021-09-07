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
    if (rt[0] != 0) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("thisis madddddddddddddddd") 
    if (rt[0] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);



    rt = (*camelCaser)("/\,.>?");
    if (rt[0] != "") {
        destroy(rt);
        return 0;
    }
    if (rt[1] != "") {
        destroy(rt);
        return 0;
    }
    if (rt[2] != "") {
        destroy(rt);
        return 0;
    }

    if (rt[3] != "") {
        destroy(rt);
        return 0;
    }

    if (rt[4] != "") {
        destroy(rt);
        return 0;
    }

    if (rt[5] != "") {
        destroy(rt);
        return 0;
    }

    if (rt[6] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);


    rt = (*camelCaser)("CS241 is Literally mad.");
    if (rt[0] != "cS241IsLiteralyMad") {
        destroy(rt);
        return 0;
    }
    if (rt[1] != NULL) {
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("WWw. WHWHWH nmmi. TREfefdds.");
    if (rt[0] != "wwww") {
        destroy(rt);
        return 0;
    }
    if (rt[1] != "whwhwhNmmi") {
        destroy(rt);
        return 0;
    }
    if (rt[2] != "trefefdds") {
        destroy(rt);
        return 0;
    }

    if (rt[3] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);


    rt = (*camelCaser)("241Nonsense.");
    if (rt[0] != "241nonsense") {
        destroy(rt);
        return 0;
    }
    if (rt[1] != NULL) {
        destroy(rt);
        return 0;
    }
    destroy(rt);

    rt = (*camelCaser)("FFFFF.  WHATAahtata is this  \n for?")
    if (rt[0] != "fffff") {
        destroy(rt);
        return 0;
    }

    if (rt[1] != "whatahtataIsThisFor") {
        destroy(rt);
        return 0;
    }

    if (rt[2] != NULL) {
        destroy(rt);
        return 0;
    }

    destroy(rt);

    rt = (*camelCaser)("变态中文测试.   Chinese test."); 
    if (rt[0] != "变态中文测试") {
        destroy(rt);
        return 0;
    }




    
    return 1;
}
