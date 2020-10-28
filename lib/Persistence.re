open Lwt.Infix;

exception PersistenceError(string);

let or_raise: result('a, [< Caqti_error.t]) => 'a =
  result =>
    switch (result) {
    | Ok(value) => value
    | Error(err) => raise(PersistenceError(Caqti_error.show(err)))
    };

let or_fail: Lwt.t(result('a, [< Caqti_error.t])) => Lwt.t('a) =
  promise =>
    promise
    >>= (
      result =>
        switch (or_raise(result)) {
        | value => Lwt.return(value)
        | exception (PersistenceError(_) as err) => Lwt.fail(err)
        }
    );

module SQLite = {
  let connect: string => Lwt.t(Caqti_lwt.connection) =
    path => {
      let uri = Printf.sprintf("sqlite3://%s", path);
      Caqti_lwt.connect(Uri.of_string(uri)) |> or_fail;
    };
};

module Postgres = {
  let connect_pool:
    (string, int, bool, string, string, string) =>
    Caqti_lwt.Pool.t(Caqti_lwt.connection, [> Caqti_error.connect]) =
    (host, port, _ssl, username, password, dbname) => {
      let uri =
        Printf.sprintf("postgres://%s:%s@%s:%d/%s", username, password, host, port, dbname);
      Caqti_lwt.connect_pool(~max_size=5, Uri.of_string(uri)) |> or_raise;
    };
};

module Elasticsearch = {};
