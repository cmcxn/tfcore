DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20250721000000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20250721000000');
-- Add your query below.


ALTER TABLE `item_instance`
  ADD COLUMN `loot_trade_expire` int(10) unsigned NOT NULL DEFAULT '0' AFTER `generated_loot`,
  ADD COLUMN `loot_trade_players` text AFTER `loot_trade_expire`;


-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
