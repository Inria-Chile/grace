/* Function composition */
let (%): ('b => 'c, 'a => 'b, 'a) => 'c = (f, g, x) => f(g(x));

/* OR for option values */
let (|?): (option('a), option('a)) => option('a) =
  (oa, ob) =>
    switch (oa, ob) {
    | (None, b) => b
    | (a, _) => a
    };

/* AND for option values */
let (|&): (option('a), option('a)) => option('a) =
  (oa, ob) =>
    switch (oa, ob) {
    | (None, _) => None
    | (_, None) => None
    | (Some(_), Some(b)) => Some(b)
    };

/* OR for result values */
let ($?): (result('a, string), result('a, string)) => result('a, string) =
  (ra, rb) =>
    switch (ra, rb) {
    | (Error(ea), Error(eb)) => Error(ea ++ "; " ++ eb)
    | (Error(_), Ok(b)) => Ok(b)
    | (Ok(a), _) => Ok(a)
    };

/* AND for result values */
let ($&): (result('a, string), result('a, string)) => result('a, string) =
  (ra, rb) =>
    switch (ra, rb) {
    | (Ok(_), Ok(b)) => Ok(b)
    | (Error(e), Ok(_)) => Error(e)
    | (Ok(_), Error(e)) => Error(e)
    | (Error(ea), Error(eb)) => Error(ea ++ "; " ++ eb)
    };
