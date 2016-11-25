
#include "define.h"
#include "bg_thread.h"
#include "parse.h"
#include "patch.h"
#include "bin.h"
#include <curl/curl.h>

static void tbl_two(HashTblEnt* ent)
{
    String* val = (String*)ent->data;
    printf("%s = %s\n", sstr_data(ent->keyStr), str_data(val));
}

static void tbl_one(void* ptr)
{
    ManifestEntry* val = (ManifestEntry*)ptr;
    printf("[%s]\n", sstr_data(val->name));
    tbl_for_each_entry(&val->content, tbl_two);
}

int main(int argc, const char** argv)
{
    if (argc > 1)
    {
        return bin_create(argc, argv);
    }
    
    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        printf("Failed to init libcurl\n");
        return 1;
    }
    
    /*if (bg_thread_start())
    {
        printf("Failed to init background thread\n");
        return 2;
    }*/
    
    sqlite3* db;
    db_init(&db);
    
    Array ar;
    HashTbl byName;
    array_init(&ar, ManifestEntry);
    tbl_init(&byName, uint32_t);
    int rc = patch_download_manifests(&ar, &byName, db);
    
    if (rc)
    {
        printf("download failed\n");
    }
    else
    {
        printf("download worked!\n");
        array_for_each(&ar, tbl_one);
    }
    
    array_deinit(&ar, parse_deinit_each_patch_entry);
    tbl_deinit(&byName, NULL);
    
    /*bg_thread_stop();*/
    curl_global_cleanup();
    
    return 0;
}
