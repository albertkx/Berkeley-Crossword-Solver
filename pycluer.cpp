// External interface to main call.

string mlg_tmpfile()
{
  char f[L_tmpnam];
  strcpy(f,"/tmp/satXXXXXX");
  close(mkstemp(f));
  return f;
}

vector<double> analyze_clues(const vector<learning_datum> &data)
{
  static const string model = "tfc.pkl";
  string query = mlg_tmpfile(), result = mlg_tmpfile();
  ofstream outf(query);
  extern string learning_headers();
  istringstream iss(learning_headers());
  string junk, tok;
  iss >> junk >> junk;
  outf << iss.str().substr(1 + iss.tellg()) << endl; // space
  for (auto &d : data) outf << d << endl;
  string cmd = "python3 predict.py " + model + " " + query + " " + result;
  system(cmd.c_str());
  vector<double> ans;
  ifstream inf(result);
  while (inf >> tok) ans.push_back(stod(tok.substr(tok[0] == '[')));
  remove(query.c_str());
  remove(result.c_str());
  assert(ans.size() == data.size());
  return ans;
}
