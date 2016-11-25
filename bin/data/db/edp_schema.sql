
-- Increase page size to speed up blob I/O
PRAGMA page_size = 8192;

CREATE TABLE info (
    key     TEXT PRIMARY KEY,
    value
);

CREATE TABLE blobs (
    id      INTEGER PRIMARY KEY,
    data    BLOB
);

CREATE TABLE applied_patches (
    id              INTEGER PRIMARY KEY,
    name            TEXT,
    description     TEXT,
    preview_url     TEXT,
    preview_blob_id INT,
    download_url    TEXT,
    version         INT
);

CREATE INDEX idx_applied_patches_name ON applied_patches (name);

CREATE TABLE affected_archives (
    id                  INTEGER PRIMARY KEY,
    path                TEXT,
    original_blob_id    INT
);

CREATE INDEX idx_affected_archives_path ON affected_archives (path);

CREATE TABLE replaced_files (
    parent_archive_id   INT,
    file_name           TEXT,
    original_blob_id    INT,
    patch_id            INT,
    
    PRIMARY KEY (parent_archive_id, file_name)
);
