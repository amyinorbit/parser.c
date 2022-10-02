# parser.c

A very basic recursive-descent parser that can lex and pare words, integers, real numbers,
and that's it. Exciting!

## Why?

Because I've written the same parser code about 20 times by now for various data formats that
all use the same basic primitives, and got tired of it. So now it's up here, I can reuse it,
and you can too!

## How

1. Drop `parser.h` and `parser.c` in your project;
2. Optionally, you may want to define `PARSE_ASSERT(expr)`, `PARSE_CALLOC(n, size)`,
   and `PARSE_FREE(ptr)` to use your own functions.
3. Profit!



## Detailed Usage

The following example parses a [GPS SEM almanac][celestrack] [file][al3]:

```c

gps_alm_t alm;

// Initialise the parser
parser_t parser = {};
parse_init(&parser, src, len);

// Do the actual parsing
alm->num_sv = parse_int(parser);
parse_text(parser, NULL, 0); // We just ignore the almanac name
alm->week = parse_int(parser);
alm->epoch = parse_int(parser);

for(int i = 0; i < alm->num_sv; ++i) {
  alm->sv[i].prn = parse_int(parser);
  alm->sv[i].svn = parse_int(parser);
  alm->sv[i].ura = parse_int(parser);
  
  alm->sv[i].ecc = parse_float(parser);
  alm->sv[i].inc = DEG2RAD(55) + M_PI * parse_float(parser);
  alm->sv[i].raan_dot = M_PI * parse_float(parser);
  alm->sv[i].sma = pow(parse_float(parser), 2);
  alm->sv[i].raan = M_PI * parse_float(parser);
  alm->sv[i].arg_p = M_PI * parse_float(parser);
  alm->sv[i].ma = M_PI * parse_float(parser);
  
  alm->sv[i].clk[0] = parse_float(parser);
  alm->sv[i].clk[1] = parse_float(parser);
  
  alm->sv[i].healthy = (parse_int(parser) == 0);
  (void)parse_int(parser); // SV conf
}

// Report errors if needed, and clean up.
if(parser.error) {
  fprintf(stderr, "parse error: %s\n", parser.error);
}
parse_fini(&parser);
```

[celestrack]: https://celestrak.org/GPS/almanac/SEM/definition.php
[al3]: https://www.navcen.uscg.gov/sites/default/files/gps/almanac/current_sem.al3
