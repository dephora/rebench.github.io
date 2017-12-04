open Rebase;

type result =
  | Ok(string)
  | Warning(string, string)
  | Error(string, list(Editor.mark))
;

module SyntaxError = {
  type location = {
    line: int,
    column: int
  };

  type range = {
    from: location,
    to_: location
  };

  type t = {
    message: string,
    range: option(range)
  };

  let fromRefmt: Refmt.error => t = e => {
    message: e##message,
    range: e##location |> Js.toOption |> Option.map(location => {
      from: {
        line: location##startLine - 2,
        column: location##startLineStartChar - 1
      },
      to_: {
        line: location##endLine - 2,
        column: location##endLineEndChar
      }
    })
  };

  [@bs.get] external loc : Js.Exn.t => {. "line": int, "column": int } = "";
  let fromAcorn = e => {
    message: e |> Js.Exn.message |> Option.getOrRaise,
    range: loc(e) |> loc => Some({
      from: {
        line: loc##line - 1,
        column: loc##column
      },
      to_: {
        line: loc##line - 1,
        column: loc##column + 1
      }
    })
  };

  let toMark = (error: t) => {
    "from": error.range |> Option.mapOr(range => {
      "line": range.from.line,
      "ch": range.from.column
    }, { "line": 0, "ch": 0 }),
    "to": error.range |> Option.mapOr(range => {
      "line": range.to_.line,
      "ch": range.to_.column
    }, { "line": 0, "ch": 1 }),
    "options": {
      "className": "syntax-error",
      "title": error.message
    }
  };
};

let _assemble = (setup, code) => {
  /*"[@bs.config {no_export: no_export}];" ++*/
  setup ++ "\n" ++ code;
};

let _check = (language, code) =>
  switch language {
  | `RE => 
    Template.apply(language, code)
      |> Refmt.parseRE
      |> Result.map2(_ast => code, e => SyntaxError.fromRefmt(e));

  | `ML => 
    Template.apply(language, code)
      |> Refmt.parseML
      |> Result.map2(ast => ast |> Refmt.printRE, e => SyntaxError.fromRefmt(e));

  | `JS =>
    switch (Acorn.parse(code)) {
    | exception Js.Exn.Error(e) =>
      Result.Error(SyntaxError.fromAcorn(e))
    | _ => {
      [%raw "0"]; /* TODO: Workaround for BS bug: https://github.com/BuckleScript/bucklescript/issues/2316 */
      Result.Ok(Template.apply(`JS, code))
    } 
    }
  };

let checkSetup = code =>
  code |> Refmt.parseRE
       |> fun | Result.Ok(ast) =>
                ast |> Refmt.printML
                    |> BS.compile
                    |> (fun | Result.Ok((code, None))           => Ok(code)
                            | Result.Ok((code, Some(warnings))) => Warning(code, warnings)
                            | Result.Error(message)             => Error(message, []))
              | Result.Error(e) =>
                e |> SyntaxError.fromRefmt
                  |> e => Error(e.message, [e |> SyntaxError.toMark]);

let compileTest = (setup, test) =>
  _check(test.Test.language, test.code)
  |> (fun | Result.Error(e) => Error(e.message, [e |> SyntaxError.toMark])
          | Result.Ok(code) =>
            _assemble(setup, code)
            |> Refmt.parseRE
            |> Result.map(Refmt.printML)
            |> (fun | Result.Error(e) => Result.Error(e##message)
                    | Result.Ok(code) => Result.Ok(code))
            |> Result.flatMap(BS.compile)
            |> (fun | Result.Ok((code, None))           => Ok(code)
                    | Result.Ok((code, Some(warnings))) => Warning(code, warnings)
                    | Result.Error(message)             => Error(message, [])));


