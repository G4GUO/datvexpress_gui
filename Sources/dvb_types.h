#ifndef DVB_TYPES_H
#define DVB_TYPES_H


typedef struct{
    short re;
    short im;
}scmplx;


//#define scmplx fftw_complex

typedef struct{
    double re;
    double im;
}dcmplx;

typedef struct{
    float re;
    float im;
}fcmplx;


#endif // DVB_TYPES_H
