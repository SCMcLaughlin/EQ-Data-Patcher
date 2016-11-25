
#include "bin.h"

const char* bin_filename(const char* str)
{
    const char* ret = str;
    uint32_t i      = 0;
    
    for (;;)
    {
        int c = str[i++];
        
        if (c == 0)
            break;
        
        if (c == '/' || c == '\\')
            ret = str + i;
    }
    
    return ret;
}

int bin_create(int argc, const char** argv)
{
    BinHeader header;
    Array subHeaders;
    Array names;
    Array files;
    uint32_t noff   = 0;
    uint32_t doff   = 0;
    int hadManifest = false;
    int rc          = ERR_None;
    BinSubHeader* shead;
    FILE* fp;
    int i;
    
    array_init(&subHeaders, BinSubHeader);
    array_init(&names, byte);
    array_init(&files, byte);
    
    for (i = 1; i < argc; i++)
    {
        const char* path    = argv[i];
        const char* name    = bin_filename(path);
        uint32_t namelen    = strlen(name);
        SimpleString* data  = sstr_from_file(path);
        BinSubHeader subhead;
        
        if (!data)
        {
            printf("Warning: could not open file '%s'; skipping\n", path);
            continue;
        }
        
        if (!hadManifest && strncmp(name, "manifest.", sizeof("manifest.") - 1) == 0)
            hadManifest = true;
        
        subhead.nameLength = namelen;
        subhead.nameOffset = noff;
        subhead.dataLength = sstr_length(data);
        subhead.dataOffset = doff;
        
        noff += namelen + 1;
        doff += sstr_length(data);
        
        if (!array_push_back(&subHeaders, &subhead))
            goto oom;
        
        if (array_append(&names, name, namelen))
            goto oom;
        
        if (array_append(&files, sstr_data(data), sstr_length(data)))
            goto oom;
        
        sstr_destroy(data);
        continue;
        
    oom:
        sstr_destroy(data);
        goto oom_err;
    }
    
    if (!hadManifest)
    {
        printf("Error: patches must contain a file called \"manifest\" (e.g. \"manifest.txt\") to define how the patch should be applied.\nNo such file was found in the provided input.\n");
        goto deinit;
    }
    
    /* Correct offsets to be relative to the start of the whole blob */
    noff    = sizeof(BinHeader) + (sizeof(BinSubHeader) * array_count(&subHeaders));
    doff    = noff + array_count(&names);
    i       = 0;
    
    while ( (shead = array_get(&subHeaders, i++, BinSubHeader)) )
    {
        shead->nameOffset += noff;
        shead->dataOffset += doff;
    }
    
    if (snprintf(header.magic, sizeof(header.magic), "EDP") <= 0)
    {
        rc = ERR_CouldNotInit;
        printf("Error: failed to write EDP file header\n");
        goto deinit;
    }
    
    header.numFiles = array_count(&subHeaders);
    
    fp = fopen("edp_patch.edp", "wb+");
    
    if (fp)
    {
        if (fwrite(&header, sizeof(BinHeader), 1, fp) != 1 ||
            fwrite(array_raw(&subHeaders), sizeof(BinSubHeader), array_count(&subHeaders), fp) != array_count(&subHeaders) ||
            fwrite(array_raw(&names), 1, array_count(&names), fp) != array_count(&names) ||
            fwrite(array_raw(&files), 1, array_count(&files), fp) != array_count(&files))
        {
            rc = ERR_FileOperation;
        }
    }
    else
    {
        rc = ERR_CouldNotOpen;
        printf("Error: could not open 'edp_patch.edp' for writing\n");
    }
    
    fclose(fp);
    
    if (!rc)
    {
        printf("Successfully created 'edp_patch.edp' with the following contents:\n\n");
        
        for (i = 1; i < argc; i++)
        {
            printf("    %s\n", bin_filename(argv[i]));
        }
        
        printf("\nYou should rename this file before uploading it somewhere.\n");
    }
    
deinit:
    array_deinit(&subHeaders, NULL);
    array_deinit(&names, NULL);
    array_deinit(&files, NULL);
    
    /* For windows: keep the console window open so the user can see the output message */
#ifdef EDP_WINDOWS
    getchar();
#endif
    
    return rc;
    
oom_err:
    printf("Error: ran out of memory\n");
    rc = ERR_OutOfMemory;
    goto deinit;
}
