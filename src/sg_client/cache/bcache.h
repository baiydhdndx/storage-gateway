/**********************************************
*  Copyright (c) 2016 Huawei Technologies Co., Ltd. All rights reserved.
*  
*  File name:    bcache.h
*  Author: 
*  Date:         2016/11/03
*  Version:      1.0
*  Description:  block write read cache
*  
*************************************************/
#ifndef SRC_SG_CLIENT_CACHE_BCACHE_H_
#define SRC_SG_CLIENT_CACHE_BCACHE_H_
#include <unistd.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "log/log.h"
#include "common/config_option.h"
#include "common/env_posix.h"
#include "../message.h"
#include "../seq_generator.h"
#include "common.h"

/*key in bcache*/
class Bkey {
 public:
    Bkey() = default;
    explicit Bkey(off_t off, size_t len, IoVersion seq);

    Bkey(const Bkey& other);
    Bkey(Bkey&& other);

    Bkey& operator=(const Bkey& other);
    Bkey& operator=(Bkey&& other);

    friend bool operator<(const Bkey& a, const Bkey& b);
    friend ostream& operator<<(ostream& cout, const Bkey& key);

    /*io write offset*/
    off_t m_off;
    /*io write length*/
    size_t m_len;
    /*io write unique sequence*/
    IoVersion m_seq;
};

struct BkeyCompare {
    bool operator()(const Bkey& a, const Bkey& b) {
        return a < b;
    }
};

typedef map<Bkey, shared_ptr<CEntry>, BkeyCompare> bcache_map_t;
typedef map<Bkey, shared_ptr<CEntry>, BkeyCompare>::iterator bcache_itor_t;

/*block read cache in memory*/
class Bcache {
 public:
    Bcache() = default;
    explicit Bcache(std::string bdev);

    Bcache(const Bcache& other) = delete;
    Bcache(Bcache&& other) = delete;

    Bcache& operator=(const Bcache& other) = delete;
    Bcache& operator=(Bcache&& other) = delete;
    ~Bcache();

    int read(off_t off, size_t len, char* buf);

    /*CRUD*/
    bool add(Bkey key, shared_ptr<CEntry> value);
    shared_ptr<CEntry> get(Bkey key);
    bool update(Bkey key, shared_ptr<CEntry> value);
    bool del(Bkey key);

    /*debug*/
    void trace();

 private:
    bcache_itor_t _data_lower_bound(off_t offset, size_t length);
    /*find which region will cache hit, and store in region_hits*/
    void _find_hit_region(off_t offset, size_t length, bcache_map_t& region_hits);
    /*find which region cache miss, and store in miss_bkeys*/
    void _find_miss_region(off_t  off,  size_t len, const vector<Bkey>& merged_bkeys,
                           vector<Bkey>& miss_bkeys);
    /*merge cache hit region, and store in merged_bkeys*/
    void _merge_hit_region(vector<Bkey> bkeys, vector<Bkey>& merged_bkeys);

    /*cache hit, read from cache*/
    int _cache_hit_read(off_t off, size_t len, char* buf,
                        const vector<Bkey>& hit_keys,
                        bcache_map_t& hit_cache_snapshot);

    /*cache misss, read from block*/
    int _cache_miss_read(off_t off, size_t len, char* buf,
                        const vector<Bkey>& miss_keys);

 private:
    std::string m_blkdev;
    unique_ptr<AccessFile> m_blkfile;
    SharedMutex  m_mutex;
    bcache_map_t m_bcache;
};

#endif  // SRC_SG_CLIENT_CACHE_BCACHE_H_
