struct Identifier
{
	unsigned int count;
	char *name;
};

struct CompletionInfo
{
	GtkTextMark *start, *end;
};

struct AutocompleteState
{
	array<Identifier *> *identifiers;

	int possible_completions_next;
	array<Identifier *> *possible_completions;

	CompletionInfo *last_completion;

	int last_cursor_offset;
};