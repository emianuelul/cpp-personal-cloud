# Cloud Storage

Built a remote cloud storage app. Both server and client. The server needs to be turned on for the client to work.

Used Slint for the GUI of the client.
nlohmann/json for json parsing.
sqlite3 to store data for files / users.
pico sha for hashing files.
tiny aes for encrypting files.
portable file dialogs to have native file pickers for when the user tries to upload a file.

## Client

The user can create an account with a unique username and password. The user can upload files to his cloud storage and
create directories. Users can of course download the uploaded files also.

## Server

Stores users and files in a sqlite data base. Stored files are encrypted and are decrypted when sending files to a
client. In case a file gets corrupted, the server repairs the file before sending it to the client.

