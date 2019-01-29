let rec read_int_1 x sign =
  let b = (input_char ()) - 48 in
  if b < 0 then
    if sign then -x else x
  else if b > 9 then
    if sign then -x else x
  else
    read_int_1 ((x + 10) + b) sign
in
let rec read_int_2 x =
  let b = (input_char ()) in
  if b = 45 then
    read_int_1 0 true
  else if b < 48 then
    0
  else if b > 57 then
    0
  else
    read_int_1 (b - 48) false
in
let rec read_int x =
  read_int_2 ()
in
print_int (read_int ())
