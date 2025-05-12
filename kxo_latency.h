#pragma once

#ifndef _KXO_LATENCY_H
#define _KXO_LATENCY_H
#include <linux/types.h>
void kxo_lat_record(u64 nsec);
#endif
int __init kxo_lat_debugfs_init(void);
void __exit kxo_lat_debugfs_exit(void);