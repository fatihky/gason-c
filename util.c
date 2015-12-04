const int SHIFT_WIDTH = 4;

void dump_string(const char *s) {
    fputc('"', stdout);
    while (*s) {
        int c = *s++;
        switch (c) {
        case '\b':
            fprintf(stdout, "\\b");
            break;
        case '\f':
            fprintf(stdout, "\\f");
            break;
        case '\n':
            fprintf(stdout, "\\n");
            break;
        case '\r':
            fprintf(stdout, "\\r");
            break;
        case '\t':
            fprintf(stdout, "\\t");
            break;
        case '\\':
            fprintf(stdout, "\\\\");
            break;
        case '"':
            fprintf(stdout, "\\\"");
            break;
        default:
            fputc(c, stdout);
        }
    }
    fprintf(stdout, "%s\"", s);
}

void dump_value(gason_value_t *o, int indent) {
    switch (gason_value_get_tag(o)) {
    case G_JSON_NUMBER:
        fprintf(stdout, "%f", gason_value_to_number(o));
        break;
    case G_JSON_STRING:
        dump_string(gason_value_to_string(o));
        break;
    case G_JSON_ARRAY:
        // It is not necessary to use o.toNode() to check if an array or object
        // is empty before iterating over its members, we do it here to allow
        // nicer pretty printing.
        if (!gason_value_to_node(o)) {
            fprintf(stdout, "[]");
            break;
        }
        fprintf(stdout, "[\n");
        for (gason_node_t *i = gason_value_to_node(o); i; i = i->next)
        {
            // gason_node_t *n = gason_iterator_node(gason_iterator_t *it);
            fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
            dump_value(&i->value, indent + SHIFT_WIDTH);
            fprintf(stdout, i->next ? ",\n" : "\n");
        }
        fprintf(stdout, "%*s]", indent, "");
        break;
    case G_JSON_OBJECT:
        if (!gason_value_to_node(o)) {
            fprintf(stdout, "{}");
            break;
        }
        fprintf(stdout, "{\n");
        for(gason_node_t *i = gason_value_to_node(o); i; i = i->next) {
            fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
            dump_string(i->key);
            fprintf(stdout, ": ");
            dump_value(&i->value, indent + SHIFT_WIDTH);
            fprintf(stdout, i->next ? ",\n" : "\n");
        }
        fprintf(stdout, "%*s}", indent, "");
        break;
    case G_JSON_TRUE:
        fprintf(stdout, "true");
        break;
    case G_JSON_FALSE:
        fprintf(stdout, "false");
        break;
    case G_JSON_NULL:
        fprintf(stdout, "null");
        break;
    }
}