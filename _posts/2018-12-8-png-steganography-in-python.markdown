---
layout: post
title: "Basic PNG Image Steganography Tool Built Using Python"
date: 2018-12-8
---

So today, I'm going to show you how I built a simple PNG steganography tool using <a href="https://python-pillow.org/">Pillow</a> and <a href="https://pypi.org/project/bitarray/">Bitarray</a> in Python 3. Here's an example of how you could encode the message "This will be encoded" into a file called "picture.png":

```
Python 3.6.6 (default, Sep 12 2018, 18:26:19) 
[GCC 8.0.1 20180414 (experimental) [trunk revision 259383]] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import py_hider # The name of my steganography tool
>>> encoded_img = py_hider.encodePNG('This will be encoded.', 'picture.png') # To encode something in PNG file
>>> encoded_img.save('encoded.png') # Save the file with the encoded message
>>> py_hider.decodePNG('encoded.png') # Decode a PNG file with a message embedded into it
'This will be encoded.'
```

Both the regular image and the image with the encoded information look almost exactly the same. You can find all of the code <a href="https://github.com/nihaal-prasad/PyHider">here</a>.

First of all, before I talk about how I coded this program, I need to explain how LSB image steganography works. Most digital images store everything in pixels, and each pixel is made up of red, green, and blue values (aka RGB values). Image steganography, as you probably already know, allows someone to hide a message inside an image so that anyone who sees the image has no clue that there is a hidden message there. I had used the least significant bits to encode my message when I created this program, which exploits the fact that humans cannot see the difference if you change the RGB values of a pixel by a 1 (ex. if you change a pixel RGB values from [123, 11, 57] to [124, 12, 58], humans will not notice a difference).

Using this technique, a programmer can therefore state that an even number in an RGB value could represent a 0 and an odd number in an RGB could represent a 1. By changing the RGB values accordingly (usually by adding one) so that their divisibility by two correspond to ones and zeros, someone could implement an ascii or utf-8 message into an image without anybody else realizing that there is a message encoded into the image. For more information on how exactly this works, click <a href="https://en.wikipedia.org/wiki/Bit_numbering#Least_significant_bit_in_digital_steganography">here</a>.

Now onto how I programmed this. The first thing I did is download <a href="https://python-pillow.org/">Pillow</a> and <a href="https://pypi.org/project/bitarray/">Bitarray</a> using the "pip3 install \<package\>" command. Next, I opened up Python and imported everything that I was going to be using. I also initialized a function called encodePNG(), which would obviously be used in order to encode the message.

```
import sys
import bitarray
from PIL import Image

def encodePNG(plaintext: str, imgFile: str, encoding='utf-8') -> Image:
```

The first thing that I needed to do inside the function was to open the image using Pillow so that I could access it.

```
    # Open the image
    img = Image.open(imgFile).convert('RGB')
```

Next, I took the plaintext, which is a parameter that is currently in string format, and converted it into a bitarray so that I could access it in 1's and 0's.

```
    # Convert the plaintext into a bitarray
    message = bitarray.bitarray()
    message.frombytes(plaintext.encode(encoding)) 
```

Then I obtained the width and height of the image in pixels, which will be required to create our two for loops.

```
    # Get the width and height of the image in pixels
    width, height = img.size
```

I created a list to keep track of the RGB data for the new image (Pillow has a function that can put the data in the RGB list into the image). A counter variable will also be used in order to keep track of how much of the message we have written.

```
    # Counter for the message
    counter = 0

    # RGB data for the new image
    new_img_data = []
```

I start two for loops in order to loop through every single pixel in the image. I also use the current x and y values to get our current RGB values, and store that information in the r, g, and b variables.

```
    # Loop through each pixel
    for y in range(0, height):
        for x in range(0, width):
            # Get the red, green, and blue values for the pixel
            r, g, b = img.getpixel((x, y))
```

I then use the least significant bit in the red value to write my message. Recall that an even number represents a zero and an odd number represents a one. I can easily change whether a number is even or odd by adding one to it. This is done in code by first doing r mod 2, which returns the least significant digit, and then adding one to r if it is not the value that I wanted it to be. I also add an else if statement to make sure that if we finish writing the message, then we can put down zeros in the least significant digit to indicate the end of message.

```
            # Continue writing our message in the red value if we have not yet finished
            if(counter < len(message)):
                if(not message[counter]):
                    if(r % 2 != 0): r = r + 1
                elif(message[counter]):
                    if(r % 2 != 1): r = r + 1
                counter = counter + 1
            elif(counter >= len(message)): # If we have finished our message, put down zeros to indicate the end of message
                if(r % 2 != 0): r = r + 1
                counter = counter + 1
```

I use the exact same code for the blue and green values.

```
            # Continue writing our message in the green value if we have not yet finished
            if(counter < len(message)):
                if(not message[counter]):
                    if(g % 2 != 0): g = g + 1
                elif(message[counter]):
                    if(g % 2 != 1): g = g + 1
                counter = counter + 1
            elif(counter >= len(message)): # If we have finished our message, put down zeros to indicate the end of message
                if(g % 2 != 0): g = g + 1
                counter = counter + 1

            # Continue writing our message in the blue value if we have not yet finished
            if(counter < len(message)):
                if(not message[counter]):
                    if(b % 2 != 0): b = b + 1
                elif(message[counter]):
                    if(b % 2 != 1): b = b + 1
                counter = counter + 1
            elif(counter >= len(message)): # If we have finished our message, put down zeros to indicate the end of message
                if(b % 2 != 0): b = b + 1
                counter = counter + 1
```

At the end of the nested for loop, I appended the new RGB values to the list that I initialized earlier.

```
            # Write down the RGB values in the new image
            new_img_data.append((r, g, b))
```

At the end of the function, I returned the new image.

```
    # Return the new image
    new_img = Image.new(img.mode, img.size)
    new_img.putdata(new_img_data)
    return new_img
```

I've finished the encodePNG() function. Now lets start working on the decodePNG() function. Like our previous function, I start by opening the image. Then I create a counter to identify a NULL character (seven 0's), which will be used to represent the end of the string. I also create a list of bits to represent the encoded message (the bitarray module has a function that can turn a list of bits into a string). I also need the width and height of the image to create the nested for loop.

```
def decodePNG(imgFile: str) -> str:
    # Open the image
    img = Image.open(imgFile).convert('RGB')

    # This counter is used to identify a NULL character (seven 0's), which will be used to represent the end of the string
    counter = 0

    # These are the bits that make up the message
    bits = []

    # Get the width and height in pixels
    width, height = img.size
```

The first part of the nested for loop is simple. Just find each RGB value, find the remainder after dividing by 2 to figure out whether it's even or odd, and then append that value to the list of bits.

```
    # Read each pixel
    for y in range(0, height):
        for x in range(0, width):
            # Get the red, green, and blue values for the pixel
            r, g, b = img.getpixel((x, y))

            # Add r to the bits list
            bits.append(r % 2)
            bits.append(g % 2)
            bits.append(b % 2) 
```

The next part involves checking whether there have been seven consecutive zeros and breaking the for loops if there has been seven consecutive zeros.

```
            # Change the counter for the r value
            if(r % 2 == 0): counter = counter + 1
            else: counter = 0

            # Change the counter for the g value
            if(g % 2 == 0): counter = counter + 1
            else: counter = 0

            # Change the counter for the b value
            if(b % 2 == 0): counter = counter + 1
            else: counter = 0

            # Check if seven zeros have been found (our terminating value) and break the inner loop if it has
            if(counter >= 7): break
        # Check if seven zeros have been found (our terminating value) and break the outer loop if it has
        if(counter >= 7): break
```

Finally, we are done with this function. All we have to do now is to convert the list of bits to a string and output this string.

```
    # Convert the message to string format and return it
    output = bitarray.bitarray(bits).tostring()
    return output[:len(output) - 1] # Slices the last character because the last character is always a null character (\x00)
```

And there you go. That's how I made a simple PNG steganography tool that works great. If you want to see all of the code, click <a href="https://github.com/nihaal-prasad/PyHider/blob/master/py_hider.py">here</a>.

{% include related_posts.html %}
