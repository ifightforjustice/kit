#ifndef KIT_OBJFILE_OSX_H_
#define KIT_OBJFILE_OSX_H_

struct databuf;
struct identmap;
struct objfile;

void osx32_flatten(struct identmap *im, struct objfile *f, struct databuf **out);

#endif /* KIT_OBJFILE_OSX_H_ */
