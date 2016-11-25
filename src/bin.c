
#include "bin.h"

const char* bin_filename(const char* str)
{
    const char* ret = str;
    uint32_t n      = strlen(str);
    uint32_t i;
    
    for (i = 0; i < n; i++)
    {
        int c = str[i];
        
        if (c == '/' || c == '\\')
            ret = str + i + 1;
    }
    
    return ret;
}

int bin_create(int argc, const char** argv)
{
    BinHeader header;
    Array subHeaders;
    Array names;
    Array files;
    uint32_t noff = 0;
    uint32_t doff = 0;
    BinSubHeader* shead;
    FILE* fp;
    int rc = ERR_None;
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
        
        subhead.nameLength  = namelen;
        subhead.nameOffset  = noff;
        subhead.dataLength  = sstr_length(data);
        subhead.dataOffset  = doff;
        
        noff += namelen + 1;
        doff += sstr_length(data);
        
        if (!array_push_back(&subHeaders, &subhead))
            goto oom;
        
        if (array_append(&names, name, namelen))
            goto oom;
        
        if (array_append(&files, sstr_data(data), sstr_length(data)))
            goto oom;
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
    
    printf("Successfully created 'edp_patch.edp' with the following contents:\n\n");
    
    for (i = 1; i < argc; i++)
    {
        printf("\t%s\n", bin_filename(argv[i]));
    }
    
    printf("\nYou should rename this file before uploading it somewhere.\n");
    
deinit:
    array_deinit(&subHeaders, NULL);
    array_deinit(&names, NULL);
    array_deinit(&files, NULL);
    
    return rc;
    
oom:
    printf("Error: ran out of memory\n");
    rc = ERR_OutOfMemory;
    goto deinit;
}
