
#include "define.h"
#include "pfs.h"
#include <curl/curl.h>

int main(void)
{
    if (curl_global_init(CURL_GLOBAL_DEFAULT))
    {
        printf("Failed to init libcurl\n");
        return 1;
    }
    
    printf("hello\n");
    
    Pfs pfs;
    pfs_init(&pfs);
    
    if (!pfs_open(&pfs, "/media/samuel/Acer/EQTitaniumClean/gfaydark.s3d"))
    {
        SimpleBuffer* buf;
        char name[] = "fayfloor.bmp";
        
        printf("opened successfully\n");
        
        buf = pfs_get(&pfs, name, sizeof(name) - 1);
        
        if (buf)
        {
            FILE* fp = fopen(name, "wb+");
            printf("got buf\n");
            
            fwrite(buf_data(buf), sizeof(byte), buf_length(buf), fp);
            fclose(fp);
            
            buf_destroy(buf);
        }
    }
    
    pfs_deinit(&pfs);
    
    CURL* curl = curl_easy_init();
    
    if (curl)
    {
        int rc;
        curl_easy_setopt(curl, CURLOPT_URL, "https://dl.dropboxusercontent.com/u/70648819/downloads/t.txt");
        rc = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        printf("curl_easy_perform() returned %i\n", rc);
    }
    
    curl_global_cleanup();
}
