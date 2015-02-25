/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/* associative array */
void assoc_init(const int hashpower_init);
CItem *assoc_find(const char *key, const size_t nkey, const uint32_t hv);
int assoc_insert(CItem *item);
void assoc_delete(const char *key, const size_t nkey, const uint32_t hv);
void do_assoc_move_next_bucket(void);
int start_assoc_maintenance_thread(void);
void stop_assoc_maintenance_thread(void);
extern unsigned int hashpower;
