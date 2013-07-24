#ifndef TRIBOX3_H_
#define TRIBOX3_H_

#ifdef __cplusplus
extern "C" {
#endif

int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3]);

#ifdef __cplusplus
}
#endif

#endif /* TRIBOX3_H_ */
