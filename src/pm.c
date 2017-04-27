#include <stdio.h>
#include <string.h>

#include <stdbool.h>

#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

static int LRU[NUM_FRAMES];
static int LRUcounter = -1;
static int loadedPage[NUM_FRAMES];

// Initialise la mémoire physique
void pm_init(FILE *backing_store, FILE *log) {
	pm_backing_store = backing_store;
	pm_log = log;
	memset(pm_memory, '\0', sizeof(pm_memory));

	for (int i; i < NUM_FRAMES; i++) {
		LRU[i] = 0;
		loadedPage[i] = -1;
	}

}

// Charge la page demandée du backing store
void pm_download_page(unsigned int page_number, unsigned int frame_number) {
	download_count++;
	/* ¡ TODO: COMPLÉTER ! */
	loadedPage[frame_number] = page_number;
	int mem_index = frame_number * PAGE_FRAME_SIZE;
	int backing_pos = page_number * PAGE_FRAME_SIZE;

	fseek(pm_backing_store, backing_pos, SEEK_SET);

	if (fread(pm_memory + mem_index, 1, PAGE_FRAME_SIZE, pm_backing_store)
			!= PAGE_FRAME_SIZE)
		perror("Cannot read from backing store.");
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page(unsigned int frame_number, unsigned int page_number) {
	backup_count++;
	int mem_index = frame_number * PAGE_FRAME_SIZE;
	int backing_pos = page_number * PAGE_FRAME_SIZE;

	fseek(pm_backing_store, backing_pos, SEEK_SET);
	if (fwrite(pm_memory + mem_index, 1, PAGE_FRAME_SIZE, pm_backing_store)
			!= PAGE_FRAME_SIZE)
		perror("Cannot write to backing store.");

}

char pm_read(unsigned int physical_address) {
	read_count++;

	return pm_memory[physical_address];
}

void pm_write(unsigned int physical_address, char c) {
	write_count++;
	pm_memory[physical_address] = c;
}

void pm_clean(void) {
	// Assurez vous d'enregistrer les modifications apportées au backing store!
	for (int i = 0; i < NUM_FRAMES; i++) {
		int backupPage = pm_getLoadedPage(i);
		if (pt_readonly_p(backupPage) == false) {
			//printf("backing up\n");
			pm_backup_page(i, backupPage);
		}
	}
	// Enregistre l'état de la mémoire physique.
	if (pm_log) {
		for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
			if (i % 80 == 0)
				fprintf(pm_log, "%c\n", pm_memory[i]);
			else
				fprintf(pm_log, "%c", pm_memory[i]);
		}
	}
	fprintf(stdout, "Page downloads: %2u\n", download_count);
	fprintf(stdout, "Page backups  : %2u\n", backup_count);
	fprintf(stdout, "PM reads : %4u\n", read_count);
	fprintf(stdout, "PM writes: %4u\n", write_count);
}

int pm_find_free_frame() {

	for (int i = 0; i < NUM_FRAMES; i++) {
		int counter = 0;
		for (int j = 0; j < PAGE_FRAME_SIZE; j++) {
			if (pm_memory[(i * PAGE_FRAME_SIZE) + j] == '\0') {
				counter++;
			}
		}
		if (counter == PAGE_FRAME_SIZE) {
			return i;
		}
	}
	return -1;
}

int pm_find_victim_pm_frame() {
	//No empty flag, find LRU frame
	int min = 0;

	for (int i = 0; i < NUM_FRAMES; i++)
	{
		if (LRU[min] > LRU[i])
			min = i;

	}
	printf("MIN: LRU[min] = %d\n", LRU[min]);

	return min;
}

//bool pm_getDirtyBit(int frame){
//	return dirtyBit[frame];
//}
//void pm_setDirtyBit(int frame, bool b){
//	dirtyBit[frame] = b;
//}
void pm_update_lru(int frame) {
	LRUcounter++;
	LRU[frame]=LRUcounter;
	return;
}
int pm_getLoadedPage(int frame) {
	return loadedPage[frame];
}
