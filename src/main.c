
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
    
    if (bg_thread_start())
    {
        printf("Failed to init background thread\n");
        return 2;
    }
    
    printf("hello\n");
    
    Array ar;
    HashTbl byName;
    array_init(&ar, ManifestEntry);
    tbl_init(&byName, uint32_t);
    int rc = patch_download_manifests(&ar, &byName);
    
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
    
    /*SimpleString* str = sstr_from_file("test.txt");
    
    Parser p;
    parse_init(&p);
    
    int rc = parse_file(&p, str);
    
    if (rc)
    {
        printf("parse failed: %i\n", rc);
    }
    else
    {
        printf("parse worked!\n");
        array_for_each(&p.content, tbl_one);
    }
    
    parse_deinit(&p);
    
    CURL* curl = curl_easy_init();
    
    if (curl)
    {
        int rc;
        curl_easy_setopt(curl, CURLOPT_URL, "https://dl.dropboxusercontent.com/u/70648819/downloads/t.txt");
        rc = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        printf("curl_easy_perform() returned %i\n", rc);
    }*/
    
    bg_thread_stop();
    curl_global_cleanup();
    
    return 0;
}
