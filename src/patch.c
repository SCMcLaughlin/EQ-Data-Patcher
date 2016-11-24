
#include "patch.h"

static size_t patch_write_manifest(char* str, size_t size, size_t count, void* ud)
{
    String* dst     = (String*)ud;
    size_t bytes    = size * count;
    
    if (bytes == 0 || str_append(dst, str, bytes))
        return 0;
    
    return bytes;
}

int patch_download_manifests(Array* patches)
{
    String accum;
    CURL* curl  = curl_easy_init();
    int rc      = ERR_None;
    
    if (!curl) return ERR_CouldNotInit;
    
    str_init(&accum);
    
    rc = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, patch_write_manifest);
    
    if (rc)
    {
    setopt_err:
        printf("Error: curl_easy_setopt() returned errcode %i\n", rc);
        rc = ERR_CouldNotInit;
        goto error;
    }
    
    rc = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &accum);
    
    if (rc) goto setopt_err;
    
    if (curl) /* make this into a loop condition */
    {
        rc = curl_easy_setopt(curl, CURLOPT_URL, "https://dl.dropboxusercontent.com/u/70648819/downloads/test.txt");
        
        if (rc) goto setopt_err;
        
        rc = curl_easy_perform(curl);
        
        if (rc)
        {
            printf("curl error: %i\n", rc);
            rc = ERR_Generic;
            goto error;
        }
        else
        {
            Parser parser;
            
            parse_init(&parser);
            rc = parse_file(&parser, str_data(&accum), str_length(&accum));
            
            if (rc)
            {
            parse_err:
                parse_deinit(&parser);
                goto error;
            }
            
            if (array_empty(patches))
            {
                array_take_ownership(patches, parse_get_manifests(&parser));
                printf("took ownership of %u patches\n", array_count(patches));
            }
            else
            {
                rc = array_append_array(patches, parse_get_manifests(&parser));
                
                if (rc) goto parse_err;
            }
            
            parse_deinit(&parser);
        }
    }
    
error:
    curl_easy_cleanup(curl);
    str_deinit(&accum);
    return rc;
}
