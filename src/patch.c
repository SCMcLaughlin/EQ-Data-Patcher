
#include "patch.h"

static size_t patch_write_manifest(char* str, size_t size, size_t count, void* ud)
{
    String* dst     = (String*)ud;
    size_t bytes    = size * count;
    
    if (bytes == 0 || str_append(dst, str, bytes))
        return 0;
    
    return bytes;
}

static int patch_set_by_names(Array* patches, HashTbl* byName)
{
    ManifestEntry* me;
    uint32_t i = 0;
    
    while ( (me = array_get(patches, i, ManifestEntry)) )
    {
        SimpleString* str = me->name;
        int rc;
        
        rc = tbl_set_str(byName, sstr_data(str), sstr_length(str), &i);
        
        i++;
        
        if (rc && rc != ERR_Again) return rc;
    }
    
    return ERR_None;
}

int patch_download_manifests(Array* patches, HashTbl* byName, sqlite3* db)
{
    String accum;
    sqlite3_stmt* stmt;
    CURL* curl  = curl_easy_init();
    int rc      = ERR_None;
    int sqlrc;
    
    if (!curl) return ERR_CouldNotInit;
    
    stmt = db_prep_literal(db, "SELECT url FROM manifest_locations ORDER BY rowid");
    
    if (!stmt)
    {
        rc = ERR_SQL;
        goto error_no_stmt;
    }
    
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
    
    for (;;)
    {
        const char* url;
        
        sqlrc = sqlite3_step(stmt);
        
        if (sqlrc == SQLITE_BUSY || sqlrc == SQLITE_LOCKED)
            continue;
        
        if (sqlrc == SQLITE_DONE)
            break;
        
        if (sqlrc != SQLITE_ROW)
            goto sql_err;
        
        url = (const char*)sqlite3_column_text(stmt, 0);
        
        if (!url)
            continue;
        
        rc = curl_easy_setopt(curl, CURLOPT_URL, url);
        
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
            }
            else
            {
                rc = array_append_array(patches, parse_get_manifests(&parser));
                
                if (rc) goto parse_err;
            }
            
            parse_deinit(&parser);
        }
    }
    
    rc = patch_set_by_names(patches, byName);
    
error:
    sqlite3_finalize(stmt);
    
error_no_stmt:
    curl_easy_cleanup(curl);
    str_deinit(&accum);
    return rc;
    
sql_err:
    printf("SQLite error (%i): '%s'\n", sqlrc, sqlite3_errstr(sqlrc));
    goto error;
}

static int patch_download(Array* patches, HashTbl* byName, SimpleString* patchName, String* download)
{
    uint32_t* index = tbl_get_str(byName, sstr_data(patchName), sstr_length(patchName), uint32_t);
    ManifestEntry* me;
    String* url;
    CURL* curl;
    int rc;
    
    if (!index) return ERR_Invalid;
    
    me = array_get(patches, *index, ManifestEntry);
    
    if (!me) return ERR_OutOfBounds;
    
    url = tbl_get_str_literal(&me->content, "url", String);
    
    if (!url) return ERR_BadUrl;
    
    curl = curl_easy_init();
    
    if (!curl)
    {
    err:
        return ERR_Generic;
    }
    
    rc = (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, patch_write_manifest)   ||
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, download)                   ||
          curl_easy_setopt(curl, CURLOPT_URL, str_data(url))                    ||
          curl_easy_perform(curl));
    
    curl_easy_cleanup(curl);
    
    if (rc) goto err;
    
    return ERR_None;
}

void patch_download_and_apply(Array* patches, HashTbl* byName, sqlite3* db, SimpleString* eqPath, RingBuf* output, SimpleString* patchName)
{
    RingPacket rp;
    String download;
    int rc;
    
    str_init(&download);
    
    /* Step 1: Download the binary patch file set */
    rc = patch_download(patches, byName, patchName, &download);
    
    if (rc)
    {
        ring_packet_init_value(&rp, RingOp_PatchDownloadFailed, rc);
        ringbuf_push(output, &rp);
        goto abort;
    }
    
    /* Step 2: Process the binary patch file set to see what's in it */
    
    /* Step 3: Find and process the manifest file to find out which local files we are changing */
    
    /*
       Step 4: For each destination archive (s3d file):
    
        1) Open the pre-existing destination file.
        2) Check if we already have an unmolested copy of this file in the db; if not, insert the file we just opened.
    
        For each file being inserted into the destination file:
    
            1) Check if the file being inserted will replace an existing file; if so, check if we have an unmolested copy of that file in the db; if not, insert it (in pfs compressed format).
            2) Insert the new file.
    
        3) Save the changes to the file on disk.
        4) Make a record of the applied patch's details in the db.
    */
    
abort:
    str_deinit(&download);
}
