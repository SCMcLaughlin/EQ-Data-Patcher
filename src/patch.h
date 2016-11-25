
#ifndef PATCH_H
#define PATCH_H

#include "define.h"
#include "structs.h"
#include "container.h"
#include "parse.h"
#include <curl/curl.h>

int patch_download_manifests(Array* patches, HashTbl* byName);

#endif/*PATCH_H*/
