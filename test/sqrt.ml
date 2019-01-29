let l =  Array.make 1 1 in
let y = l.(0) in
let rec min_caml_sqrt x =
  let x2 = x *. 0.5 in
  let i = union_ftoi x in
  let i = i / 2 in
  let i = 1597463007 - i in
  let f = union_itof i in
  let f = f *. (1.5 -. (x2 *. f *. f)) in
  let f = f *. (1.5 -. (x2 *. f *. f)) in
  1.0 /. f
in
print_int (int_of_float ((min_caml_sqrt 1600.0) *. 100000.0));
print_newline ();
print_int (int_of_float ((sqrt 1600.0) *. 100000.0))
