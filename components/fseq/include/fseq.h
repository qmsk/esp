#pragma once

#include <stdio.h>

struct fseq;

int fseq_new(struct fseq **fseqp, FILE *file);
