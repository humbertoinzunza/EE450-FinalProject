Student Name:   Humberto Inzunza Velarde
USCID:          2693018599

The project is complete as established by the guidelines given. Additionally,
I finished the extra credit section using the SHA256 algorithm.

Files included:

    backend_server.h: Used for the declaration of the BackendServer class,
    including its data members and member functions.

    backend_server.cpp: Manipulates the data members and defines the member
    functions of the BackendServer class. It is based on the behavior of
    backend servers S, D, and U as established in the project description.

    serverS.h: Header file for the ServerS class. Inherits from BackendServer.

    serverS.cpp: Contains the main method where a ServerS object is instantiated
    and an infinite receive loop is executed.

    serverD.h: Header file for the ServerD class. Inherits from BackendServer.

    serverD.cpp: Contains the main method where a ServerD object is instantiated
    and an infinite receive loop is executed.

    serverU.h: Header file for the ServerU class. Inherits from BackendServer.

    serverU.cpp: Contains the main method where a ServerU object is instantiated
    and an infinite receive loop is executed.

    serverM.h: Used for the declaration of the ServerM class, including
    its data members and member functions.

    serverM.cpp: Defines the behavior of the ServerM class. Its main method
    contains an infinite accept loop that forks when a new connection is made.

    client.h: Used for the declaration of the Client class, including
    its data members and member functions.

    client.cpp: Defines the behavior of the Client class. Its main method contains
    an infinite authenticate loop that only finishes when the user is authenticated
    as a guest or member. It also contains an infinite loop for continuous requests.

    extra_member.txt: Input file containing the hash code of 10 login credentials
    formatted as <username SHA256 hash>, <password SHA256 hash>.

    extra_member.txt: Input file containing 10 unencrypted login credentials
    formatted as <username>, <password>.

Messages:

    Initial Data (Backend Server to Main Server):

        Backend Server to Main Server:

        <room code (2 bytes)> <room status (2 bytes)> This repeats back to back for every entry in the backend server.

    Authentication:

        Client to Main Server:
        
        <request code (1 byte)> <encrypted username> <null character (1 byte)> <encrypted password (if any)>

        Main Server to Client:

        <response code (1 byte)>
    
    SHA256 Authentication:

        Client to Main Server:

        <request code (1 byte)> <encrypted username (32 bytes)> <encrypted password (32 bytes)> (password is sent as all 0s for guests)

        Main Server to Client:

        <response code (1 byte)>
    
    
    Availability and Reservation:

        Client to Main Server:
        
        <request code (1 byte)> <room code>

        Main Server to Backend Server:
        
        <request code (1 byte)> <room code (2 bytes)>

        Backend Server to Main Server:

        <response code (1 byte)>

        Main Server to Client:

        <response code (1 byte)>

Possible points of failure:

    The different executables should not fail as long as the inputs are consistent
    with the project description (input files format, room format, password and
    username length, etc.). Also, room number and its status can be a max of 2^16 - 1.

Reused code:

    client.cpp contains the sha256() function. This function was written from scratch using
    online examples to debug. However, the initial values of the hash array and the array of
    constants were both obtained from https://www.rfc-editor.org/rfc/rfc4634.html.
    These values are defined as:
    Initial hash value: "first 32 bits of the fractional parts of the square roots of the first 8 primes (2..19)"
    Initialize array K: "first 32 bits of the fractional parts of the cube roots of the first 64 primes (2..311)"

    backend_server.cpp, serverM.cpp, and client.cpp all define the functions get_addrinfos()
    and create_sockets(). These functions include the use of getaddrinfo(), bind(), and
    connect() which were obtained from Beej’s Guide to Network Programming.

    serverM.cpp's main method contains an accept loop which uses the accept() and fork()
    commands. The basic structure of this loop was obtained from Beej’s Guide to Network
    Programming.

    serverM.cpp also contains the listen_for_connections() function that uses the command
    listen() and contains a small section of code to reap dead processes (requires the
    sigchld_handler() function). This code was also obtained from Beej’s Guide to Network
    Programming.

    Finally, every send(), recv(), sendto(), and recvfrom() follow a similar format (like
    the example below) and all these basic calls were also obtained from Beej’s Guide to
    Network Programming.
    format:
    if ((numbytes = sendto(socketfd, out_buffer, numbytes, 0, ptr_to_address, ptr_to_address_length)) == -1) {
        perror("send");
        exit(some error code);
    }

Extra credit:

    The encryption algorithm I chose is SHA256.

    Brief description:

    SHA256 stands for Secure Hashing Algorithm and the 256 indicates the hash length in bits.
    It is a fast, secure, and deterministic algorithm that is used in electronic signatures
    (like with download verification), password hashing (though deliberately slow algorithms
    may deliver better security against dictionary attacks), and in blockchain. 
    The algorithm is not too complicated, but it's hard too debug due to its many "moving parts"
    as seen in step 5.

    How to run:

    To run the program with this encryption algorithm you must add the command -e after
    ./client and ./serverM, so you run the main server as ./serverM -e and the client as
    ./client -e. Any other command sent (or no command at all) will use the basic encryption
    algorithm.

    How the program uses the algorithm:

    There are two pre-made input files: extra_member.txt and extra_member_unencrypted.txt
    with 10 login credentials encrypted and unencrypted, respectively. The client will send
    the encrypted hash of both the username and password (an array of 8 32-bit elements each).
    The main server will receive these values and convert them to string in their hexadecimal
    representation to compare with the values in extra_member.txt.

    SHA256 Steps:

    1. Split the UTF-8 encoded string into 512-bit (64-byte) blocks. If the input size is not
    a multiple of 512-bits (very likely), then the input must be padded at the last block to
    achieve this size. For the purposes of this program the algorithm only uses one block
    (password max of 50 characters). The last 8 bytes (64-bits) of the last block are filled
    with the size of the message in bits.

    2. Initialize the message schedule array (64 elements, 32-bit each) to zero. Copy the
    512 bits of the block to the array so that the first 16 elements of the message schedule
    are "filled".

    3. Calculate alpha_0 as -> right rotate of message_schedule[1] by 7 XOR right rotate of
    message_schedule[1] by 18 XOR right shift of message_schedule[1] by 3. Similarly,
    alpha_1 is calculated using message_schedule[14] and rotating 17 instead of 7, 19 instead
    of 18 and shifting 10 instead of 3.
    With alpha_0 and alpha_1 calculate message_schedule[17] = message_schedule[0] + alpha_0 +
    message_schedule[9] +  alpha_1. In future iterations calculate message_schedule indices 18
    through 63 using the same indices used in the calculation but incremented 1 (0 to 1, 1 to 2, 9 to 10, 14 to 15).

    4. Initialize the hash value array (8 elements, 32-bit each) to the "first 32 bits of the
    fractional parts of the square roots of the first 8 primes 2..19)". Initialize the constant
    array K with the "first 32 bits of the fractional parts of the cube roots of the first 64
    primes 2..311". Initialize the working variables (a through h) to hash[0] through hash[7].

    5. In a loop that repeats 64 times (because it uses message_schedule and the constant array K)
    calculate:
        Majority as (a AND b) XOR (a AND c) XOR (b AND c)
        Choice as (e AND f) XOR ((NOT e) AND g)
        Sigma_0 as (Right rotate a by 2) XOR (Right rotate a by 13) XOR (Right rotate a by 22)
        Sigma_1 as (Right rotate e by 6) XOR (Right rotate e by 11) XOR (Right rotate e by 25)
        Temp_1 as h + Sigma_1 + Choice + K[current_index] + message_schedule[current_index]
        Temp_2 = Sigma_0 + Majority
    And update the working variables as:
        h = g;
        g = f;
        f = e;
        e = d + Temp_1;
        d = c;
        c = b;
        b = a;
        a = Temp_1 + Temp_2;
    
    6. Finally, add the working variables to the hash array like: hash[0] += a, hash[1] += b, ...,
    hash[7] += h. The array hash is the final hash (256 bits).