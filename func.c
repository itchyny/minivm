static void f_min(env* e, value* values, int len) {
  int i; long l; double d, g;
  if (len == 0) {
    printf("Invalid argument for min()\n");
    exit(1);
  }
  value v;
  v.type = VT_LONG;
  for (i = 0; i < len; ++i) {
    if (v.type == VT_LONG) {
      if (values[i].type == VT_LONG || values[i].type == VT_BOOL) {
        l = TO_LONG(values[i]);
        v.lval = i == 0 ? l : l > v.lval ? v.lval : l;
        continue;
      }
      v.type = VT_DOUBLE;
      d = (double)v.lval;
    } else {
      d = v.dval;
    }
    g = TO_DOUBLE(values[i]);
    v.dval = i == 0 ? g : d > g ? g : d;
  }
  e->stack[e->stackidx++] = v;
}

static void f_max(env* e, value* values, int len) {
  int i; long l; double d, g;
  value v;
  if (len == 0) {
    printf("Invalid argument for max()\n");
    exit(1);
  }
  v.type = VT_LONG;
  for (i = 0; i < len; ++i) {
    if (v.type == VT_LONG) {
      if (values[i].type == VT_LONG || values[i].type == VT_BOOL) {
        l = TO_LONG(values[i]);
        v.lval = i == 0 ? l : l > v.lval ? l : v.lval;
        continue;
      }
      v.type = VT_DOUBLE;
      d = (double)v.lval;
    } else {
      d = v.dval;
    }
    g = TO_DOUBLE(values[i]);
    v.dval = i == 0 ? g : d > g ? d : g;
  }
  e->stack[e->stackidx++] = v;
}

static void f_abs(env* e, value* values, int len) {
  value v;
  if (len != 1) {
    printf("Invalid argument for abs()\n");
    exit(1);
  }
  v = values[0];
  if (v.type == VT_DOUBLE) {
    e->stack[e->stackidx++].dval = v.dval >= 0.0 ? v.dval : -v.dval;
  } else {
    long l = TO_LONG(v);
    e->stack[e->stackidx].type = VT_LONG;
    e->stack[e->stackidx++].lval = l >= 0 ? l : -l;
  }
}

func gfuncs[] = {
  { "abs", f_abs },
  { "min", f_min },
  { "max", f_max },
};
