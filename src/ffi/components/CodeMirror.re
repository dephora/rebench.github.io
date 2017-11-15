open Rebase;

[@bs.module "react-codemirror2"] external codeMirror : ReasonReact.reactClass = "UnControlled";
[@bs.send] external focus : ReasonReact.reactRef => unit = "";

let execCommand: (ReasonReact.reactRef, string) => unit = [%bs.raw {|
  function (el, command) {
    return el.getCodeMirror().execCommand(command);
  }
|}];

let make =
    (
      ~style: option(Js.t({..}))=?,
      ~value: option(string)=?,
      ~defaultValue: option(string)=?,
      ~onChange: option(string => unit)=?,
      ~options: option(Js.t({..}))=?,
      children
    ) =>
  ReasonReact.wrapJsForReason(
    ~reactClass=codeMirror,
    ~props={
      "style": Js.Undefined.from_opt(style),
      "value": Js.Undefined.from_opt(value),
      "defaultValue": Js.Undefined.from_opt(defaultValue),
      "onChange": onChange |> Option.mapOr(f => (_, _, value) => f(value), (_, _, _) => ()),
      "options": Js.Undefined.from_opt(options)
    },
    children
  );
