extends Tile

# ---- CONSTANTES ----
const DEVICE_NAME = "sensor_a"
const SUB_HB = "hb_" + DEVICE_NAME
const SUB_STATUS_LED = "status_" + DEVICE_NAME + "_led"
const SUB_CMD_SET_LED = "cmd_" + DEVICE_NAME + "_set_led"
## Délai sans heartbeat au-delà duquel le capteur est considéré comme bloqué (3,5 périodes de 1 s).
const HB_TIMEOUT_S := 3.5
## Durée d'allumage de l'icône de heartbeat à chaque réception.
const HB_BLINK_S := 0.5
## Définition de chaque couleur

const LED_COLORS = {
	"Off": [0, 0, 0],
	"White": [255, 255, 255],
	"Red": [255, 0, 0],
	"Blue": [0, 0, 255],
	"Green": [0, 255, 0],
	"Yellow": [255, 255, 0],
	"Orange": [255, 57, 0],
	"Violet": [255, 0, 255]
}


# ---- VARIABLES ----
var _node_id: int = -1
var _is_on: bool = false
var _seconds_since_hb := INF
# Variable "_connected" avec setter personnalisé pour appeler la MAJ de l'UI au changement d'état
var _connected: bool = false:
	set(value):
		if value == _connected: return
		_connected = value
		if _connected:
			_on_sensor_connected()
		else:
			_on_sensor_disconnected()

func _ready() -> void:
	super()
	%LedButton.pressed.connect(_on_switch_led)
	%SetColorButton.pressed.connect(_on_change_color_clicked)

func get_subscriptions() -> PackedStringArray:
	var subscriptions: PackedStringArray = []
	subscriptions.append(SUB_HB)
	subscriptions.append(SUB_STATUS_LED)
	return subscriptions

## Source principale du node_id : la passerelle annonce le capteur par son nom dès son enregistrement,
## avant ses premières publications (dont l'état republié dans onSessionOpened côté Arduino).
func on_node_online(node_id: int, _mac_address: String, device_name: String) -> void:
	if device_name != DEVICE_NAME: return
	_node_id = node_id  # nouvel identifiant possible après un redémarrage du capteur
	%IDLabel.text = "node_id: %d" % _node_id


func on_node_offline(node_id: int, _reason_code: int) -> void:
	if node_id == _node_id: _connected = false


func on_gateway_reachability_changed(is_gateway_reachable: bool) -> void:
	if is_gateway_reachable: return
	_connected = false
	_node_id = -1  # la passerelle peut redistribuer les identifiants en redémarrant


func on_message(subject_name: String, source_id: int, _payload: PackedByteArray) -> void:
	if subject_name == SUB_HB: _on_hb(source_id)
	if source_id != _node_id: return # ne traitez que les messages dédiés à votre capteur
	# Ajoutez vos messages à la suite
	if subject_name == SUB_STATUS_LED: _on_status_led(_payload)


func _process(delta: float) -> void:
	_seconds_since_hb += delta
	%HeartbeatIcon.state = _seconds_since_hb < HB_BLINK_S
	if _connected and _seconds_since_hb >= HB_TIMEOUT_S:
		_connected = false  # plus de heartbeat : code du capteur bloqué ou capteur hors de portée


func _on_sensor_connected():
	%IDLabel.text = "node_id: %d" % _node_id
	%ConnectedIcon.state = true


func _on_sensor_disconnected():
	%IDLabel.text = "node_id: ∅"
	%ConnectedIcon.state = false

## @brief Traite un heartbeat : (re)connecte le capteur et relance la temporisation.
## @param source_id  Identifiant de l'appareil émetteur.
func _on_hb(source_id: int):
	# Secours si la tuile n'a pas vu on_node_online (IHM lancée après le capteur, par exemple)
	if _node_id == -1: _node_id = source_id
	if source_id != _node_id: return
	_seconds_since_hb = 0.0
	_connected = true


func _on_status_led(payload: PackedByteArray) -> void: # quand le statut de la led est changé (démarrage / chang. couleur)
	if payload.size() != 3: return
	_is_on = payload[0] > 0 or payload[1] > 0 or payload[2] > 0
	%LedLabel.set("theme_override_colors/font_color", Color(payload[0], payload[1], payload[2])) # l'ui affiche la couleur courante
	if payload[0] == LED_COLORS["Off"][0] && payload[1] == LED_COLORS["Off"][1] && payload[2] == LED_COLORS["Off"][2]: # on met à jour le texte du label
		%LedLabel.text = "Off";
	elif payload[0] == LED_COLORS["White"][0] && payload[1] == LED_COLORS["White"][1] && payload[2] == LED_COLORS["White"][2]:
		%LedLabel.text = "White";
	elif payload[0] == LED_COLORS["Red"][0] && payload[1] == LED_COLORS["Red"][1] && payload[2] == LED_COLORS["Red"][2]:
		%LedLabel.text = "Red";
	elif payload[0] == LED_COLORS["Blue"][0] && payload[1] == LED_COLORS["Blue"][1] && payload[2] == LED_COLORS["Blue"][2]:
		%LedLabel.text = "Blue";
	elif payload[0] == LED_COLORS["Green"][0] && payload[1] == LED_COLORS["Green"][1] && payload[2] == LED_COLORS["Green"][2]:
		%LedLabel.text = "Green";
	elif payload[0] == LED_COLORS["Yellow"][0] && payload[1] == LED_COLORS["Yellow"][1] && payload[2] == LED_COLORS["Yellow"][2]:
		%LedLabel.text = "Yellow";
	elif payload[0] == LED_COLORS["Orange"][0] && payload[1] == LED_COLORS["Orange"][1] && payload[2] == LED_COLORS["Orange"][2]:
		%LedLabel.text = "Orange";
		%LedLabel.set("theme_override_colors/font_color", Color("Orange")) # le orange reconstitué via le payload semble jaune dans le texte
	elif payload[0] == LED_COLORS["Violet"][0] && payload[1] == LED_COLORS["Violet"][1] && payload[2] == LED_COLORS["Violet"][2]:
		%LedLabel.text = "Violet";
	
func _on_change_color_clicked():
	if _is_on:
		var random = randi() % 7 # random de 0 à 6 inclus
		var color = ""
		match random:
			0:
				color = "White";
			1:
				color = "Red";
			2:
				color = "Blue";
			3:
				color = "Green";
			4:
				color = "Yellow";
			5:
				color = "Orange";
			6:
				color = "Violet";
		var new_color = PackedByteArray(LED_COLORS[color]) # on envoie une couleur aléatoire au capteur
		publish(SUB_CMD_SET_LED, new_color)
	else:
		return
	
func _on_switch_led():
	print("Switch led")
	if _is_on == true: # si led allumé, on l'éteint, sinon on l'allume
		print("switch true")
		publish(SUB_CMD_SET_LED, PackedByteArray(LED_COLORS["Off"]))
	else:
		print("switch false")
		publish(SUB_CMD_SET_LED, PackedByteArray(LED_COLORS["White"]))
