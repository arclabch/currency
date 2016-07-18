/*
 * Copyright © 2016 ARClab, Lionel Riem - https://arclab.ch/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>

#include "config.h"


/*
 * currency -- converts one currency into another
 */

int
main(int argc, char *argv[])
{
    uint8_t i;
    off_t fileSize, fileSizeControl;
    int statReturn;
    bool cacheRefresh, cacheError, readError, rateFromError, rateToError;
    struct stat cacheStat;
    struct timespec currentTime;
    time_t diffTime;
    char curFrom[4], curTo[4], *filePath, *apiUrl, *json;
    double curAmount, curResult;
    CURL *curl;
    CURLcode res;
    FILE *f;
    struct json_object *rates, *rateFrom, *rateTo;

    /* If arguments not as expected, display usage. */
    if(argc != 4) {
        fprintf(stderr, "currency -- Currency converter.\n");
        fprintf(stderr, "Usage:   currency FROM TO amount\n");
        fprintf(stderr, "Example: currency USD EUR 123.45\n");
        return EXIT_SUCCESS;
    }

    /* Copy From and To and uppercase them. */
    for(i=0;i<3;i++) {
        curFrom[i] = toupper(argv[1][i]);
        curTo[i]   = toupper(argv[2][i]);
    }
    curFrom[3] = '\0';
    curTo[3]   = '\0';

    /* Convert amount from string to float. */
    curAmount = atof(argv[3]);

    /* Check if cache exists and if it needs a refresh (every 1h). */
    cacheRefresh = true;
    cacheError   = false;
    (void)clock_gettime(CLOCK_REALTIME, &currentTime);
    asprintf(&filePath, "%s/%s", getenv("HOME"), FILE_NAME);
    statReturn = stat(filePath, &cacheStat);
    if(statReturn != -1) {
        diffTime = currentTime.tv_sec - cacheStat.st_mtim.tv_sec;
        if (diffTime < 3600) {
            cacheRefresh = false;
        }
    }

    /* If needed, refresh cache. */
    if(cacheRefresh == true) {
        asprintf(&apiUrl, "%s?app_id=%s", API_URL, API_KEY);
        curl = curl_easy_init();
        if(curl) {
            f = fopen(filePath, "w");
            curl_easy_setopt(curl, CURLOPT_URL, apiUrl);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "currency.c/" VERSION);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
            res = curl_easy_perform(curl);
            if(res != 0) {
                cacheError = true;
            }
            curl_easy_cleanup(curl);
            fclose(f);
        } else {
            cacheError = true;
        }
        free(apiUrl);
        if(cacheError == true) {
            fprintf(stderr, "Warning: unable to refresh currency rates. ");
            fprintf(stderr, "Trying to use previous data.\n");
        }
    }

    /* Load currencies file to memory. */
    readError = false;
    fileSize = 0;
    statReturn = stat(filePath, &cacheStat);
    if(statReturn == 0) {
        fileSize = cacheStat.st_size;
    }
    f = fopen(filePath, "r");
    if(f == NULL) {
        readError = true;
    } 
    json = (char *)malloc(fileSize+1);
    fileSizeControl = fread(json, sizeof(char), fileSize, f);
    if (fileSize != fileSizeControl) { 
        readError = true;
    } 
    fclose(f);
    free(filePath);
    json[fileSize] = '\0';

    if(readError == true) {
        fprintf(stderr, "Error: unable to read currency rates. ");
        fprintf(stderr, "Please verify that the file exists and try again.\n");
        free(json);
        return EXIT_FAILURE;
    }

    /* Extract the rates. */
    rates = json_tokener_parse(json);
    json_object_object_get_ex(rates, "rates", &rates);

    rateFromError = json_object_object_get_ex(rates, curFrom, &rateFrom);
    rateToError   = json_object_object_get_ex(rates, curTo,   &rateTo);

    if(rateFromError == false) {
        fprintf(stderr, "Error: '%s' is not recognized as a currency.\n",
                curFrom);
        free(json);
        return EXIT_FAILURE;
    }

    if(rateToError == false) {
        fprintf(stderr, "Error: '%s' is not recognized as a currency.\n",
                curTo);
        free(json);
        return EXIT_FAILURE;
    }

    /* Calculate the result, display, and great success! */
    curResult = curAmount / json_object_get_double(rateFrom);
    curResult = curResult * json_object_get_double(rateTo);

    printf("%s %0.4f = %s %0.4f\n", curFrom, curAmount, curTo, curResult);

    free(json);
    return EXIT_SUCCESS;
}
