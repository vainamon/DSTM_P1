int main(){
 int i, *j, k;
 j = &k;
 __tm_atomic{
   *j = i++;
 }
 return 0;
}
