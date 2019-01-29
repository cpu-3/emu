let y = ref 1 in
  let g y = y > 0 in
  let rec f x = f x in
  let x = g !y || f true in
  x
