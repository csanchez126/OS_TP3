#include <stdio.h>
#include <string.h>

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
static int LRUcounter = 0;


// Initialise la mémoire physique
void pm_init(FILE *backing_store, FILE *log) {
	pm_backing_store = backing_store;
	pm_log = log;
	memset(pm_memory, '\0', sizeof(pm_memory));

	for(int i; i<NUM_FRAMES; i++){
		LRU[i] = 0;
	}

}

// Charge la page demandée du backing store
void pm_download_page(unsigned int page_number, unsigned int frame_number) {
	download_count++;
	/* ¡ TODO: COMPLÉTER ! */

	int mem_index = frame_number * PAGE_FRAME_SIZE;
	int backing_pos = page_number * PAGE_FRAME_SIZE;

	fseek(pm_backing_store, backing_pos, SEEK_SET);

	if (fread(pm_memory + mem_index, 1, PAGE_FRAME_SIZE, pm_backing_store)
			!= PAGE_FRAME_SIZE)
		perror("Cannot read from backing store.");

	//For testing
//	for (int i = mem_index; i < mem_index + PAGE_FRAME_SIZE; i++) {
//		printf("CHAR %d: %c\n", i, pm_memory[i]);
//	}

}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_frame(unsigned int frame_number, unsigned int page_number) {
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
	LRUcounter++;
	if(physical_address==0){
		LRU[0] = LRUcounter;
	}
	else{
		LRU[NUM_FRAMES/physical_address] = LRUcounter;
	}
	return pm_memory[physical_address];
}

void pm_write(unsigned int physical_address, char c) {
	write_count++;
	LRUcounter++;
	if(physical_address==0){
		LRU[0] = LRUcounter;
	}
	else{
		LRU[NUM_FRAMES/physical_address] = LRUcounter;
	}
	pm_memory[physical_address] = c;
}

void pm_clean(void) {
	// Assurez vous d'enregistrer les modifications apportées au backing store!
	/* ¡TODO: On doit looper pour backup chaque frame readonly du PT ? !*/
	for(int i=0; i<NUM_PAGES; i++){

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

int find_victim_pm_frame(){
	int emptyFlag = -1;
	char emptyFrame[PAGE_FRAME_SIZE];
	char aFrame[PAGE_FRAME_SIZE];

	memset(emptyFrame, '\0', sizeof(emptyFrame));
	//Check if a frame is free
	for(int i=0 ; i < NUM_FRAMES; i+=PAGE_FRAME_SIZE){
		strncpy(aFrame, pm_memory+(i*PAGE_FRAME_SIZE), PAGE_FRAME_SIZE);
		if(strcmp(aFrame, emptyFrame) == 0){
			return i; //Found an empty frame
		}else{
			emptyFlag = i;
			break;
		}
	}
	//No empty flag, find LRU frame
	int min = 0;
	if(emptyFlag > -1){
		for(int i=0; i<NUM_FRAMES; i++){
			if(LRU[min] < LRU[i]){
				min = i;
			}
		}
	}else{
		printf("Free frame error");
	}

	return min;
}
