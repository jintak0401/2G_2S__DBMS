SELECT name
FROM Pokemon
WHERE type = 'grass'
ORDER BY name;

SELECT name
FROM Trainer
WHERE hometown = 'Rainbow City' OR hometown = 'Brown City'
ORDER BY name;

SELECT DISTINCT type
FROM Pokemon
ORDER BY type;

SELECT name
FROM City
WHERE name LIKE 'B%'
ORDER BY name;

SELECT hometown
FROM Trainer
WHERE name NOT LIKE 'M%'
ORDER BY hometown;

SELECT nickname
FROM CatchedPokemon
WHERE level = (
  SELECT MAX(level)
  FROM CatchedPokemon
  )
ORDER BY nickname;

SELECT name
FROM Pokemon
WHERE name LIKE 'a%' OR name LIKE 'e%' OR name LIKE 'i%' OR name LIKE 'o%' OR name LIKE 'u%'
ORDER BY name;

SELECT AVG(level)
FROM CatchedPokemon;

SELECT MAX(level)
FROM CatchedPokemon, Trainer
WHERE owner_id = Trainer.id AND Trainer.name = 'Yellow';

SELECT DISTINCT hometown
FROM Trainer
ORDER BY hometown;

SELECT name, nickname
FROM CatchedPokemon, Trainer
WHERE owner_id = Trainer.id AND nickname LIKE 'A%'
ORDER BY name;

SELECT name
FROM Trainer
WHERE id = (
  SELECT leader_id
  FROM Gym, City
  WHERE description = 'Amazon' AND name = city
  );

SELECT t.name, COUNT(cp.id) AS numOfFire
FROM Trainer t, CatchedPokemon cp
JOIN Pokemon AS p ON cp.pid = p.id AND p.type = 'Fire'
WHERE t.id = cp.owner_id
GROUP BY t.name
ORDER BY COUNT(cp.id)
LIMIT 1;

SELECT DISTINCT type
FROM Pokemon
WHERE id < 10
ORDER BY id DESC;

SELECT COUNT(*)
FROM Pokemon
WHERE type <> 'Fire';

SELECT name
FROM Pokemon, Evolution
WHERE before_id = id AND before_id > after_id
ORDER BY name;

SELECT AVG(level)
FROM CatchedPokemon, Pokemon p
WHERE pid = p.id AND p.type = 'Water';

SELECT nickname
FROM CatchedPokemon
JOIN Gym AS g ON g.leader_id = owner_id
WHERE level = (
  SELECT MAX(level)
  FROM CatchedPokemon, Gym
  WHERE leader_id = owner_id
  );

SELECT n.name
FROM (
  SELECT t.name, AVG(cp.level) AS average
  FROM Trainer t, CatchedPokemon cp
  WHERE t.hometown = 'Blue City' AND t.id = cp.owner_id
  GROUP BY t.name) AS n
WHERE n.average = (
  SELECT MAX(nest.average)
  FROM (
    SELECT t.name, AVG(cp.level) AS average
    FROM Trainer t, CatchedPokemon cp
    WHERE t.hometown = 'Blue City' AND t.id = cp.owner_id
    GROUP BY t.name) AS nest
  )  
ORDER BY n.name;

SELECT p.name
FROM Pokemon p, CatchedPokemon cp
JOIN Trainer AS t ON t.id = cp.owner_id 
  AND t.id IN (
    SELECT id
    FROM Trainer
    GROUP BY hometown
    HAVING COUNT(hometown) = 1
    )
WHERE p.id = cp.pid AND p.type = 'Electric'
  AND p.id IN (
    SELECT before_id
    FROM Evolution
    );

SELECT t.name, SUM(cp.level)
FROM Trainer t, CatchedPokemon cp
JOIN Gym ON leader_id = cp.owner_id
WHERE cp.owner_id = t.id
GROUP BY t.name
ORDER BY SUM(cp.level) DESC;

SELECT hometown
FROM Trainer
GROUP BY hometown
ORDER BY COUNT(hometown) DESC
LIMIT 1;

SELECT DISTINCT name
FROM Pokemon
WHERE name IN (
  SELECT p.name
  FROM Pokemon p, CatchedPokemon cp
  JOIN Trainer AS t ON cp.owner_id = t.id AND t.hometown = 'Sangnok City'
  WHERE p.id = cp.pid
  ) AND name IN (
  SELECT p.name
  FROM Pokemon p, CatchedPokemon cp
  JOIN Trainer AS t ON cp.owner_id = t.id AND t.hometown = 'Brown City'
  WHERE p.id = cp.pid
  )
ORDER BY name;

SELECT t.name
FROM Trainer t, CatchedPokemon cp
JOIN Pokemon AS p ON p.id = pid AND p.name LIKE 'P%'
WHERE t.id = cp.owner_id AND t.hometown = 'Sangnok City'
ORDER BY t.name;

SELECT t.name, p.name
FROM Trainer t, Pokemon p, CatchedPokemon cp
WHERE t.id = cp.owner_id AND p.id = cp.pid
ORDER BY t.name, p.name;

SELECT p.name
FROM Pokemon p, Evolution e
WHERE p.id = e.before_id AND 
  e.after_id NOT IN (
    SELECT before_id
    FROM Evolution
    ) AND
  e.before_id NOT IN (
    SELECT after_id
    FROM Evolution
    )
ORDER BY p.name;

SELECT cp.nickname
FROM Trainer t, Gym g, CatchedPokemon cp
JOIN Pokemon AS p ON p.id = cp.pid AND p.type = 'Water'
WHERE cp.owner_id = t.id AND t.id = g.leader_id AND g.city = 'Sangnok City'
ORDER BY nickname;

SELECT t.name
FROM Trainer t, CatchedPokemon cp
WHERE cp.pid IN (
  SELECT after_id
  FROM Evolution
  ) AND t.id = cp.owner_id
GROUP BY t.name
HAVING COUNT(cp.pid) >= 3
ORDER BY t.name;


SELECT name
FROM Pokemon
WHERE id NOT IN (
  SELECT pid
  FROM CatchedPokemon
  )
ORDER BY name;

SELECT cp.level
FROM Trainer t, (
  SELECT t.hometown, cp.level
  FROM Trainer t, CatchedPokemon cp
  WHERE t.id = cp.owner_id
  ORDER BY cp.level DESC) AS cp
GROUP BY cp.hometown
ORDER BY cp.level DESC;

SELECT p.id AS ID, p.name AS first, (
	SELECT p2.name
	FROM Pokemon p2
	WHERE p2.id = (
		SELECT after_id
		FROM Evolution e2
		WHERE e2.before_id = p.id
		)
	) AS second,
	(
	SELECT p3.name
	FROM Pokemon p3
	WHERE p3.id = (
		SELECT e3.after_id
		FROM Evolution e3
		WHERE e3.before_id = (
			SELECT id
            FROM Pokemon
            WHERE name = second
            )
		)
	) AS third
FROM Pokemon p, Evolution e
WHERE p.id = e.before_id AND e.after_id IN (
	SELECT before_id
	FROM Evolution
	)
ORDER BY p.id;