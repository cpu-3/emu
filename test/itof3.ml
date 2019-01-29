let rec min_caml_itof x =
  let y = itof_inner (if x < 0 then -x else x) in
  if x < 0 then -1.0 *. y else y
in
let rec min_caml_ftoi x =
  let y = ftoi_inner (if x < 0.0 then -1.0 *. x -. 0.5 else x -. 0.5) in
  if x < 0.0 then -y else y
in
print_int (min_caml_ftoi (min_caml_itof (-10)))
