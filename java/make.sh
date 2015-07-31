echo "compiling..."
rm -r net
javac -encoding ISO-8859-1 -d . TTLexEntry.java
javac -encoding ISO-8859-1 -d . TTLexEntryToObj.java
javac -encoding ISO-8859-1 -d . TTPNode.java
javac -encoding ISO-8859-1 -d . TT.java
javac -encoding ISO-8859-1 -d . TTConnection.java
javac -encoding ISO-8859-1 -d . Example.java
javac -encoding ISO-8859-1 -d . Parse.java
javac -encoding ISO-8859-1 -d . HowManyMinutesIs.java
javac -encoding ISO-8859-1 -d . WhereDoIFind.java
echo "making JAR..."
rm -f tt.jar
jar cvf tt.jar com
echo "making documentation..."
rm -r doc
javadoc -encoding ISO-8859-1 -d doc -public TT*.java
