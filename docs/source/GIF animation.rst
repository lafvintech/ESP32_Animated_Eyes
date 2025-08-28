.. __GIF animation

GIF animation
========================================

The ESP32_Animated_Eyes features a GIF mode. By default, the provided folder contains 8 GIF images. Users can use a card reader to store the images on a TF card. After inserting the TF card into the TF module and powering on the device again, it will enter GIF mode and start displaying the GIF images.
   .. image:: /Tutorial/img/G1.png
   .. image:: /Tutorial/img/LA065_A6.jpg

If you want to customize and display your own favorite images, we also provide a way to convert MP4 files to GIF format:

1. Prepare the MP4 video you want to display.
2. Please click to enter `https://ezgif.com/video-to-gif <https://ezgif.com/video-to-gif>`_
3. Upload the MP4 video file you want to use.

   .. image:: /Tutorial/img/G2.png
   .. image:: /Tutorial/img/G3.png
   .. image:: /Tutorial/img/G4.png

4. Adjust the parameters you need (try to shorten the duration and reduce the number of frames, otherwise it may cause slow loading and running times or even failure).
   
   .. image:: /Tutorial/img/G5.png

5. Adjust the resolution of the image (since the screen used is a 1.28-inch round one, if you want the image to be centered, the ratio of length to width should be 1:1. It is recommended to use 240x240, but you can make appropriate adjustments without making it too large).

   .. image:: /Tutorial/img/G6.png
   .. image:: /Tutorial/img/G7.png

6. To ensure the smooth operation of the GIF image, it is necessary to optimize its size.

   .. image:: /Tutorial/img/G8.png
   .. image:: /Tutorial/img/G11.png

7. Compress the file size to around 300K to 500K before saving it.(The name for saving should be in the format of number.gif. You can also adjust the playback order by modifying the name, but be careful not to repeat any names.)
   
   .. image:: /Tutorial/img/G9.png
   .. image:: /Tutorial/img/G10.png

Finally, remove the TF card and insert it into the TF card module. Power on the device again to enter GIF mode.

.. raw:: html

   <div style="border: 2px solid red; padding: 10px; background-color: #ffcccc;">

**Note:**

* **1.** GIFs not generated using this method may not be able to be parsed.
* **2.** If it still fails to run, it might be because the GIF is too large. You can refer to the various options provided on the website to compress it.

.. raw:: html

   </div>