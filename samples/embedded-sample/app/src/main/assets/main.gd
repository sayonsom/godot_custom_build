extends Control

var counter: int = 0

func _ready():
	var lottie = $ScrollContainer/VBox/Margin/Content/LottieAnim
	var status = $ScrollContainer/VBox/Margin/Content/StatusLabel
	var counter_label = $ScrollContainer/VBox/Margin/Content/CounterBox/CounterLabel
	var text_input = $ScrollContainer/VBox/Margin/Content/TextInput

	# Lottie info
	status.text = "Frames: %d | Duration: %.1fs | Size: %s" % [
		lottie.get_total_frames(),
		lottie.get_duration(),
		str(lottie.get_animation_size())
	]

	# Lottie controls
	$ScrollContainer/VBox/Margin/Content/LottieControls/PlayBtn.pressed.connect(func():
		lottie.play()
		status.text = "Status: Playing"
	)
	$ScrollContainer/VBox/Margin/Content/LottieControls/PauseBtn.pressed.connect(func():
		lottie.pause()
		status.text = "Status: Paused"
	)
	$ScrollContainer/VBox/Margin/Content/LottieControls/StopBtn.pressed.connect(func():
		lottie.stop()
		status.text = "Status: Stopped"
	)

	# Lottie signals
	lottie.animation_looped.connect(func():
		print("Animation looped")
	)
	lottie.animation_finished.connect(func():
		status.text = "Status: Finished"
		print("Animation finished")
	)

	# Announce button
	$ScrollContainer/VBox/Margin/Content/AnnounceBtn.pressed.connect(func():
		var msg = "Lottie is %s. Counter is %d." % [
			"playing" if lottie.is_playing() else "stopped",
			counter
		]
		status.text = msg
		print("Announced: ", msg)
	)

	# Counter buttons
	$ScrollContainer/VBox/Margin/Content/CounterBox/IncrementBtn.pressed.connect(func():
		counter += 1
		counter_label.text = "Count: %d" % counter
	)
	$ScrollContainer/VBox/Margin/Content/CounterBox/DecrementBtn.pressed.connect(func():
		counter -= 1
		counter_label.text = "Count: %d" % counter
	)

	# Text input
	text_input.text_submitted.connect(func(text):
		status.text = "You typed: %s" % text
		print("Text submitted: ", text)
	)

	print("Test app ready!")
	print("  Lottie frames: ", lottie.get_total_frames())
	print("  Lottie duration: ", lottie.get_duration())
	print("  Lottie size: ", lottie.get_animation_size())
	print("  Lottie playing: ", lottie.is_playing())
