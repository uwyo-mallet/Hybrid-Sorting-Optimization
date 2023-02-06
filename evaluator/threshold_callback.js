const t = cb_obj.value;
let new_source = {};

const keys = Object.keys(source.data);
for (let k of keys) {
  new_source[k] = [];
}

for (let i = 0; i < source.data.threshold.length; ++i) {
  if (source.data.threshold[i] != t) {
    continue;
  }

  for (let k of keys) {
    new_source[k].push(source.data[k][i]);
  }
}

source.data = new_source;
