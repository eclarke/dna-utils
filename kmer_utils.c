// Copyright 2013 Calvin Morrison
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kmer_total_count.h"

const unsigned char alpha[256] =
{5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 5, 1, 5, 5, 5, 2, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 0, 5, 1, 5, 5, 5, 2,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

const char reverse_alpha[4] = { 'A', 'C', 'G', 'T' };

inline unsigned long long pow_four(unsigned long long x) {
  return (unsigned long long)1 << (x * 2);
}

// convert a string of k-mer size base-4 values  into a
// base-10 index
inline unsigned long num_to_index(const char *str, const int kmer, const long error_pos) {
  int i = 0;
  unsigned long out = 0;
  unsigned long multiply = 1;

  for(i = kmer - 1; i >= 0; i--){

    if(str[i] >> 2) {
#ifndef SHARED
      position += i;
#endif
      return error_pos;
    }

    out += str[i] * multiply;
    multiply = multiply << 2;
  }

  return out;
}

// convert an index back into a kmer string
char *index_to_kmer(unsigned long long index, long kmer)  {
  int i = 0;
  int j = 0;
  char *num_array = calloc(kmer,  sizeof(char));
  char *ret = calloc(kmer + 1, sizeof(char));
  if(ret == NULL)
    exit(EXIT_FAILURE);


  // this is the core of the conversion. modulus 4 for base 4 conversion
  while (index != 0) {
    num_array[i] = index % 4;
    index /= 4;
    i++;
  }

  // for our first few nmers, like AAAAA, the output would only be "A" instead
  // of AAAAA so we prepend it
  for(j = 0; j < (kmer - i); j++)
    ret[j] = 'A';

  // our offset for how many chars we prepended
  int offset = j;
  // save i so we can print it
  int start = i ;

  // decrement i by 1 to reverse the last i++
  i--;
  j = 0;

  // reverse the array, as j increases, decrease i
  for(j = 0; j < start; j++, i--)
    ret[j + offset] = reverse_alpha[(int)num_array[i]];

  // set our last character to the null termination byte
  ret[kmer + 1] = '\0';

  free(num_array);
  return ret;
}

// Strip out any character 'c' from char array 's' into a destination dest (you
// need to allocate that) and copy only len characters.
char *strnstrip(const char *s, char *dest, int c, unsigned long long len) {
  unsigned long long i = 0;
  unsigned long long j = 0;

  for(i = 0; i < len; i++) {
    if(s[i] != c) {
      dest[j] = s[i];
      j++;
    }
  }

  dest[j] = '\0';

  return dest;
}

unsigned long long * get_kmer_counts_from_file(const char *fn, const unsigned int kmer) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  long long i = 0;
  long long position = 0;

  FILE * const fh = fopen(fn, "r");
  if(fh == NULL) {
    fprintf(stderr, "Error opening %s - %s\n", fn, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // width is 4^kmer
  // there's a sneaky bitshift to avoid pow dependency
  const unsigned long width = pow_four(kmer);

  // malloc our return array
  unsigned long long * counts = calloc((width+ 1), sizeof(unsigned long long));
  if(counts == NULL)  {
    fprintf(stderr, strerror(errno));
    exit(EXIT_FAILURE);
  }

  char *str = malloc(4096);
  if(str == NULL) {
    fprintf(stderr, strerror(errno));
    exit(EXIT_FAILURE);
  }

  unsigned long long str_size = 4096;

  while ((read = getdelim(&line, &len, '>', fh)) != -1) {

    // find our first \n, this should be the end of the header
    char *start = strchr(line, '\n');
    if(start == NULL)
      continue;

    // point to one past that.
    start = start + 1;

    size_t start_len = strlen(start);


    // if our current str buffer isn't big enough, realloc
    if(start_len + 1 > str_size + 1) {
      str = realloc(str, start_len + 1);
      if(str == NULL) {
        exit(EXIT_FAILURE);
        fprintf(stderr, strerror(errno));
      }
    }


    // strip out all other newlines to handle multiline sequences
    str = strnstrip(start, str, '\n',start_len);
    size_t seq_length = strlen(str);

    // relace A, C, G and T with 0, 1, 2, 3 respectively
    // everything else is 5
    for(i = 0; i < seq_length; i++) {
      str[i] = alpha[(int)str[i]];
    }

    // loop through our string to process each k-mer
    for(position = 0; position < (seq_length - kmer + 1); position++) {
      unsigned long mer = 0;
      unsigned long multiply = 1;

      // for each char in the k-mer check if it is an error char
      for(i = position + kmer - 1; i >= position; i--){
        if(str[i] == 5) {
          mer = width;
          position = i;
          goto next;
        }

        // multiply this char in the mer by the multiply
        // and bitshift the multiply for the next round
        mer += str[i] * multiply;
        multiply = multiply << 2;
      }
      // use this point to get mer of our loop
    next:
      // bump up the mer value in the counts array
      counts[mer]++;
    }
  }

  free(line);
  free(str);
  fclose(fh);

  return counts;
}
