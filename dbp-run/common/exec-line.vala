public class ExecLine {
	private string _str;
	private string[] _exec;
	
	public string str {
		get {return _str;}
	}
	public string[] exec {
		get {return _exec;}
	}
	
	private enum ParseState {
		NORMAL,
		QUOTE,
		BACKSLASH,
		PERCENT
	}
	
	public ExecLine(string cmd_line) {
		string[] arguments = {};
		StringBuilder arg = new StringBuilder();
		unichar c;
		ParseState state = ParseState.NORMAL;
		int i;
		
		_str = cmd_line;
		
		for(i = 0; cmd_line.get_next_char (ref i, out c);) {
			switch(state) {
				case ParseState.NORMAL:
				case ParseState.QUOTE:
					switch(c) {
						case '"':
							state = (state == ParseState.QUOTE) ? ParseState.NORMAL : ParseState.QUOTE;
							break;
						
						case '\\':
							state = ParseState.BACKSLASH;
							break;
						
						case '%':
							state = ParseState.PERCENT;
							break;
						
						case ' ':
							if(state == ParseState.QUOTE) {
								arg.append_unichar(c);
							} else {
								if(arg.str.length >0) {
									arguments += arg.str;
								}
								arg = new StringBuilder();
							}
							break;
						
						default:
							arg.append_unichar(c);
							break;
					}
					break;
					
				case ParseState.BACKSLASH:
					arg.append_unichar(c);
					state = ParseState.NORMAL;
					break;
					
				case ParseState.PERCENT:
					switch(c) {
						case '%':
							arg.append_unichar(c);
							break;
					}
					state = ParseState.NORMAL;
					break;
			}
		}
		
		if(arg.str.length > 0) {
			arguments += arg.str;
		}
		
		_exec = arguments;
	}
	
	public void append(string[] args) {
		foreach(string s in args) {
			_exec += s;
		}
	}
	
	public void run(bool in_background) throws SpawnError {
		if(in_background)
			Process.spawn_async(null, _exec, null, SpawnFlags.SEARCH_PATH | SpawnFlags.CHILD_INHERITS_STDIN, null, null);
		else
			Process.spawn_sync(null, _exec, null, SpawnFlags.SEARCH_PATH | SpawnFlags.CHILD_INHERITS_STDIN, null, null, null);
	}
	
	public void print() {
		stdout.printf("[\n");
		foreach(string s in _exec)
			stdout.printf("\t%s\n", s);
		stdout.printf("]\n");
	}
}
