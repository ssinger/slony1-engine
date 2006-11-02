BEGIN { 
  f=0; 
  a["major"] = -1; 
  a["minor"] = -1; 
  a["patchlevel"] = -1; 
}

/create or replace function \@NAMESPACE\@.slonyVersion/ {f=f+1}
/slonyVersionMajor/ {v="major"}
/slonyVersionMinor/ {v="minor"}
/slonyVersionPatchlevel/ {v="patchlevel"}

/return / {
  if ((f > 0) && (f < 4)) {
    s=$0; 
    gsub("return", "", s);   
    gsub(" ", "", s);     
    gsub(";", "", s);     
    gsub("\t", "", s);	     
    if (a[v] < 0) {                
      a[v] = s}
  }
} 

END {
  printf "%s.%s.%s", a["major"], a["minor"], a["patchlevel"];
}
