
#include "define.h"
#include "bg_thread.h"
#include <curl/curl.h>

int main(void)
{
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
    
    CURL* curl = curl_easy_init();
    
    if (curl)
    {
        int rc;
        curl_easy_setopt(curl, CURLOPT_URL, "https://dl.dropboxusercontent.com/u/70648819/downloads/t.txt");
        rc = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        printf("curl_easy_perform() returned %i\n", rc);
    }
    
    bg_thread_stop();
    curl_global_cleanup();
    
    return 0;
}
