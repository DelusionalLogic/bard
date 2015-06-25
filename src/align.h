#ifndef ALIGN_H
#define ALIGN_H

static const char* const AlignStr[] = {
	"%{l}",
	"%{c}",
	"%{r}",
};

enum Align {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

#define ALIGN_NUM ((int)ALIGN_RIGHT+1)
#define ALIGN_FIRST ((int)ALIGN_LEFT)
#define ALIGN_LAST ((int)ALIGN_RIGHT)

#endif
