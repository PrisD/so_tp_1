/*
 * sm1.c
 * 
 * Copyright 2024 osboxes <osboxes@osboxes>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Simulador Memoria paginada simple (sin memoria virtual)
 * 
 * Forma de compilacion:
 * $ gcc -Wall -o sm1 sm1.c
 * 
 * Forma de uso:
 * $ ./sm1
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// arma menu y devuelve opcion elegida
int menu(const char *titulo,const char *opciones[]);
// funcion generica para ingreso de datos
void ingreso(char *buffer,int size);
// pide confirmacion al usuario
int ingreso_sino(char *msg);
// ingresa un numero entero
int ingreso_int();

struct bcp {
	int pid;
	int tam_pro;
	int npag_pro;
	int *tabla_pagina;
} bcp;

struct data_frame {
	unsigned char libre; // 0 libre   1 ocupado
	unsigned char so;    // 0 usuario 1 so 
} data_frame;

// datos para gestion de memoria
struct data_mem {
	int tam_memoria;
	int tam_so;
	int tam_pagina;
	int nframe_so; // cantidad de frames que ocupa el so, desde el 0 hasta el (nframe_so - 1)
	int nframe; // cantidad total de frames en memoria principal
	int nframe_libres; // nro de marcos libres
	struct data_frame *fr; // puntero a nframe de tipo data_frame o NULL
	int nproc; // cantidad de procesos actual
	struct bcp *bcp; // puntero al bcp de todos los procesos
} data_mem;


// funciones 
// crea tabla de frames y hace calculos, requiere tam mem, tam so, tam pag previamente cargados
void crear_tabla_frames(struct data_mem *);
// mostrar datos
void mostrar_data(struct data_mem *,int);
// ocupa marcos de memoria
void ocupar_frame(struct data_mem *,int nmarco,unsigned char so);

int main(int argc, char **argv)
{
	const char *opc[8] = { "Tamaño memoria Real", "Tamaño SO", "Tamaño Pagina", "Ejecutar Proceso",
						   "Terminar Proceso","Mostrar Datos", "Mapeo Logico-Fisco", NULL };
	int opcion=0;
	struct data_mem mem;
	memset(&mem,0,sizeof(struct data_mem)); // mem=0
	
	do {
		opcion = menu("SIMULADOR ADM MEMORIA PAGINADA SIMPLE",opc);
		switch(opcion) {
			case 1:
				printf("Ingrese (en KB) tamaño memoria real:");
				mem.tam_memoria = ingreso_int();
				break;
			case 2:
				printf("Ingrese (en KB) tamaño SO:");
				mem.tam_so = ingreso_int();
				break;
			case 3:
				printf("Ingrese (en KB) tamaño pagina:");
				mem.tam_pagina = ingreso_int();
				crear_tabla_frames(&mem);
				mostrar_data(&mem,0);
				break;
			case 4: // ejecutar proceso
				printf("Ingrese el PID del proceso:");
				int ppid = ingreso_int();
				printf("Ingrese (en KB) tamaño proceso:");
				int tpro = ingreso_int();
				int npag_pro = tpro / mem.tam_pagina;
				if (tpro % mem.tam_pagina) npag_pro++;
				// valido
				if ( mem.nframe_libres < npag_pro ) {
					printf("main(): No hay espacio suficiente para este proceso!\n");
				} else {
					// hay espacio!
					mem.nproc++;
					if ( mem.bcp ) mem.bcp = (struct bcp *) realloc(mem.bcp,sizeof(struct bcp) * mem.nproc);
					else mem.bcp = (struct bcp *) malloc(sizeof(struct bcp) * mem.nproc);
					struct bcp *pp = mem.bcp+(mem.nproc-1);
					pp->pid = ppid;
					pp->tam_pro = tpro;
					pp->npag_pro = npag_pro;
					// creamos tabla de pagina
					pp->tabla_pagina = (int *) malloc(sizeof(int)*pp->npag_pro);
					// asignar marcos libres a este proceso
					struct data_frame *pf = mem.fr+mem.nframe_so;
					int *ppag = pp->tabla_pagina;
					int i=mem.nframe_so, canp = 0;
					for(;i<mem.nframe && canp < pp->npag_pro;i++,pf++) { // recorro la tabla de marcos
						if ( pf->libre == 0 ) {
							// encontre marco libre
							*ppag = i;
							ocupar_frame(&mem,i,0);
							canp++;
							ppag++;
						}
					}
					mem.nframe_libres-=pp->npag_pro;
					mostrar_data(&mem,0);
				}
				break;
			case 99:
				if ( !ingreso_sino("Realmente quiere salir?") ) opcion = 0;
				break;
		}
	} while(opcion != 99);
	
	// libero memoria
	if ( mem.fr ) free(mem.fr); 
	if ( mem.bcp ) {
		int ii;
		struct bcp *pbcp = mem.bcp;
		for(ii=0;ii<mem.nproc;ii++,pbcp++) {
			if ( pbcp->tabla_pagina ) free(pbcp->tabla_pagina);
		}
		free(mem.bcp);
	}
	
	return 0;
}


// arma menu y devuelve opcion elegida
int menu(const char *titulo,const char *opciones[]) {
	int op,i,opp=0;
	do {
		op=1;i=0;
		printf("\t\t%s\n",titulo);
		while(opciones[i]) {
			printf("\t%2.0d. %s\n",op,opciones[i]);
			op++;i++;
		}
		printf("\t99. Salir\n");
		printf("Ingrese su opcion:");
		opp=ingreso_int();
		if ( opp == 99 ) return 99;
	} while(opp <= 0 || opp > (op-1));
	return opp;
}

// funcion generica para ingreso de datos
void ingreso(char *buffer,int size) {
	fgets(buffer,size,stdin);
	buffer[strlen(buffer)-1]='\0';
}

// ingresar si / no
int ingreso_sino(char *msg) {
	char rta[4];
	do {
		printf("%s (si/no)",msg);
		ingreso(rta,4);
	} while(!(strcasecmp(rta,"si") == 0 || strcasecmp(rta,"no") == 0));
	if ( strcasecmp(rta,"si") == 0 ) return 1;
	else return 0;
}

int ingreso_int() {
	char codigo[20];
	ingreso(codigo,20);
	return atoi(codigo);
}


// crea tabla de frames y hace calculos, requiere tam mem, tam so, tam pag previamente cargados
void crear_tabla_frames(struct data_mem *m) {
	if ( m->tam_memoria && m->tam_so && m->tam_pagina ) {
		//
		m->nframe = m->tam_memoria / m->tam_pagina;
		m->nframe_so = m->tam_so / m->tam_pagina;
		if (m->tam_so % m->tam_pagina) m->nframe_so++;
		m->nframe_libres = m->nframe - m->nframe_so;
		// libero memoria
		if ( m->fr ) free(m->fr); 
		m->fr = (struct data_frame *) malloc(sizeof(struct data_frame) * m->nframe);
		memset(m->fr,0,sizeof(struct data_frame) * m->nframe); // m->fr = 0;
		// ocupemos los  marcos del SO
		int i;
		for(i=0;i < m->nframe_so;i++) ocupar_frame(m,i,1);
		
	} else printf("crear_tabla_frames(): Error, faltan datos para calculo!\n");
}


// mostrar datos
// recibe puntero de tipo struct data_mem que aputa a los datos
// y muestra de acuerdo a opcion (1 -> muestro tabla de marcos, 2 -> muestro..
void mostrar_data(struct data_mem *m,int opcion) {
	int i;
	printf("Tam. Memoria: %d\nTam. SO: %d\nTam. Pag: %d\nN frames SO: %d\nN frames Mem: %d\nN frames libres: %d\n",
		m->tam_memoria,m->tam_so,m->tam_pagina,m->nframe_so,m->nframe,m->nframe_libres);
	// mostrar marcos
	if ( m->fr ) {
		struct data_frame *f = m->fr;
		for(i=0;i < m->nframe; i++, f++) {
			printf("Frame %d Estado: %d Tipo: %d\n",i,(int) f->libre,(int) f->so);
		} 
	}
	// mostrar bcp
	if ( m->bcp ) {
		struct bcp *p = m->bcp;
		for(i=0;i < m->nproc; i++, p++) {
			printf("Proceso %d Tam: %d KB n Pag: %d\n",p->pid,p->tam_pro,p->npag_pro);
			if ( p->tabla_pagina ) {
				int ii;
				for(ii=0;ii<p->npag_pro;ii++) 
					printf("\t#Pag %d Frame %d\n",ii,*(p->tabla_pagina+ii));
			}
		}
	}
}

// ocupa el marco (base 0) de memoria nmarco como so
void ocupar_frame(struct data_mem *m,int nmarco,unsigned char so) {
	(m->fr+nmarco)->libre = 1; // marco ocupado
	(m->fr+nmarco)->so = so; // marco del so o del usuario?
}
